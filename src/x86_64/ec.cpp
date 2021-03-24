/*
 * Execution Context (EC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
 *
 * This file is part of the NOVA microhypervisor.
 *
 * NOVA is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NOVA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License version 2 for more details.
 */

#include "abi.hpp"
#include "counter.hpp"
#include "ec_arch.hpp"
#include "elf.hpp"
#include "extern.hpp"
#include "fpu.hpp"
#include "hip.hpp"
#include "interrupt.hpp"
#include "sm.hpp"
#include "space_hst.hpp"
#include "space_obj.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Ec::cache { sizeof (Ec_arch), Kobject::alignment };

Atomic<Ec *>    Ec::current     { nullptr };
Ec *            Ec::fpowner     { nullptr };
unsigned        Ec::donations   { 0 };

// Factory: Kernel Thread
Ec *Ec::create (unsigned c, cont_t x)
{
    return new (cache) Ec_arch (c, x);
}

// Factory: User Thread
Ec *Ec::create (Status &s, Pd *pd, bool fpu, unsigned c, unsigned long e, bool t, uintptr_t a, uintptr_t sp)
{
    auto const obj { pd->get_obj() };
    auto const hst { pd->get_hst() };
    auto const pio { pd->get_pio() };

    if (EXPECT_FALSE (!obj || !hst || (Ec_arch::needs_pio && !pio))) {
        s = Status::ABORTED;
        return nullptr;
    }

    // FIXME: Refcount updates

    auto const f { fpu ? new (pd->fpu_cache) Fpu : nullptr };
    auto const u { new Utcb };
    Ec *ec;

    if (EXPECT_TRUE ((!fpu || f) && u && (ec = new (cache) Ec_arch (obj, hst, pio, f, u, c, e, t, a, sp))))
        return ec;

    delete u;
    Fpu::operator delete (f, pd->fpu_cache);

    s = Status::MEM_OBJ;

    return nullptr;
}

void Ec::create_idle()
{
    Status s;
    current = Ec::create (Cpu::id, idle);
    Scheduler::set_current (Pd::create_sc (s, &Space_obj::nova, Space_obj::Selector::NOVA_CPU + Cpu::id, current, Cpu::id, 1000, 0, 0));
}

void Ec::create_root()
{
    auto const ra { *reinterpret_cast<uintptr_t *>(Kmem::sym_to_virt (&__boot_ra)) };

    if (EXPECT_FALSE (!ra)) {
        trace (TRACE_ROOT, "ROOT: No image");
        return;
    }

    Status s;

    Pd::root = Pd::create (s);

    auto const obj { Pd::root->create_obj (s, &Space_obj::nova, Space_obj::Selector::ROOT_OBJ) };
    auto const hst { Pd::root->create_hst (s, &Space_obj::nova, Space_obj::Selector::ROOT_HST) };
                     Pd::root->create_pio (s, &Space_obj::nova, Space_obj::Selector::ROOT_PIO);

    obj->delegate (&Space_obj::nova, Space_obj::Selector::NOVA_OBJ, Space_obj::selectors - 1, 0, Capability::pmask);
    obj->delegate (&Space_obj::nova, Space_obj::Selector::ROOT_OBJ, Space_obj::selectors - 2, 0, Capability::pmask);
    obj->insert (Space_obj::selectors - 3, Capability (Pd::root, std::to_underlying (Capability::Perm_pd::DEFINED)));

    auto info_addr { (Space_hst::selectors() - 1) << PAGE_BITS };
    auto utcb_addr { (Space_hst::selectors() - 2) << PAGE_BITS };

    auto const ec { Pd::create_ec (s, obj, Space_obj::selectors - 4, Pd::root, Cpu::id, utcb_addr, 0, 0, BIT (2) | BIT (0)) };
    auto const sc { Pd::create_sc (s, obj, Space_obj::selectors - 5, ec, Cpu::id, 1000, Scheduler::priorities - 1, 0) };

    if (EXPECT_FALSE (!ec || !sc))
        return;

    auto const e { static_cast<Eh const *>(Hptp::map (ra)) };

    if (EXPECT_FALSE (!e->valid (Eh::ELF_MACHINE))) {
        trace (TRACE_ROOT, "ROOT: Wrong architecture");
        return;
    }

    auto const abi { Sys_abi (ec->sys_regs()) };

    abi.p0() = *reinterpret_cast<uintptr_t *>(Kmem::sym_to_virt (&__boot_p0));
    abi.p1() = *reinterpret_cast<uintptr_t *>(Kmem::sym_to_virt (&__boot_p1));
    abi.p2() = *reinterpret_cast<uintptr_t *>(Kmem::sym_to_virt (&__boot_p2));

    ec->cont = Ec_arch::ret_user_hypercall;
    ec->exc_regs().ip() = e->entry;
    ec->exc_regs().sp() = info_addr;

    uint64 root_e { 0 }, root_s { root_e - 1 };

    auto c { __atomic_load_n (&e->ph_count, __ATOMIC_RELAXED) };

    for (auto p { static_cast<Ph const *>(Hptp::map (ra + e->ph_offset)) }; c--; p++) {

        if (p->type != 1)
            continue;

        auto perm { Paging::Permissions (Paging::R  * !!(p->flags & BIT (2)) |
                                         Paging::W  * !!(p->flags & BIT (1)) |
                                         Paging::XU * !!(p->flags & BIT (0)) |
                                         Paging::U) };

        trace (TRACE_ROOT | TRACE_PARSE, "ROOT: P:%#llx => V:%#llx PM:%#x FS:%#llx MS:%#llx", p->f_offs + ra, p->v_addr, perm, p->f_size, p->m_size);

        if (p->f_size != p->m_size || p->v_addr % PAGE_SIZE != (p->f_offs + ra) % PAGE_SIZE)
            return;

        uint64 phys { align_dn (p->f_offs + ra, PAGE_SIZE) };
        uint64 virt { align_dn (p->v_addr, PAGE_SIZE) };
        uint64 size { align_up (p->v_addr + p->f_size, PAGE_SIZE) - virt };

        root_s = min (root_s, phys);
        root_e = max (root_e, phys + size);

        for (unsigned o; size; size -= BITN (o), phys += BITN (o), virt += BITN (o))
            hst->delegate (&Space_hst::nova, phys >> PAGE_BITS, virt >> PAGE_BITS, (o = static_cast<unsigned>(min (max_order (phys, size), max_order (virt, size)))) - PAGE_BITS, perm, Memattr::ram());
    }

    Hip::hip->build (root_s, root_e);

    hst->update (info_addr, Kmem::ptr_to_phys (Hip::hip), 0, Paging::Permissions (Paging::K | Paging::U | Paging::R), Memattr::ram());

    Scheduler::unblock (sc);

    Console::flush();
}

void Ec::activate()
{
    auto ec { this };

    for (donations = 0; ec->callee; ec = ec->callee)
        donations++;

    // Fast path: EC is not blocked (and has no chance to block).
    // Slow path: EC may be unblocked from a remote core anytime.
    if (EXPECT_TRUE (!ec->blocked() || !ec->block_sc()))
        static_cast<Ec_arch *>(ec)->make_current();
}

void Ec::help (Ec *ec, cont_t c)
{
    if (EXPECT_FALSE (ec->cont == dead))
        return;

    cont = c;

    // Preempt long helping chains, including livelocks
    Cpu::preemption_point();
    if (EXPECT_FALSE (Cpu::hazard & Hazard::SCHED))
        Scheduler::schedule (false);

    Counter::helping.inc();

    ec->activate();

    Scheduler::schedule (true);
}

void Ec::idle (Ec *const self)
{
    trace (TRACE_CONT, "%s", __func__);

    for (;;) {

        auto hzd { Cpu::hazard & (Hazard::RCU | Hazard::SLEEP | Hazard::SCHED) };
        if (EXPECT_FALSE (hzd))
            self->handle_hazard (hzd, idle);

        Cpu::halt();
    }
}

void Ec::kill (char const *reason)
{
    trace (TRACE_KILL, "Killed EC:%p (%s)", static_cast<void *>(this), reason);

    auto const ec { caller };

    if (ec)
        ec->cont = ec->cont == Ec_arch::ret_user_hypercall ? sys_finish<Status::ABORTED> : dead;

    reply (dead);
}

/*
 * Switch FPU ownership
 *
 * @param ec    Prospective new owner (or nullptr to unassign FPU)
 * @return      True if FPU ownership was changed, false otherwise
 */
bool Ec::switch_fpu (Ec *ec)
{
    // We want to assign the FPU to an EC
    if (EXPECT_TRUE (ec)) {

        // The EC must not be the FPU owner
        assert (fpowner != ec);

        // The FPU must be disabled
        assert (!(Cpu::hazard & Hazard::FPU));

        // The EC is not eligible to use the FPU
        if (EXPECT_FALSE (!ec->fpu))
            return false;
    }

    Fpu::enable();

    // Save state of previous owner
    if (EXPECT_TRUE (fpowner))
        fpowner->fpu_save();

    fpowner = ec;

    // Load state of new owner
    if (EXPECT_TRUE (fpowner))
        fpowner->fpu_load();

    return true;
}
