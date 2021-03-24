/*
 * Execution Context (EC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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
#include "multiboot.hpp"
#include "sm.hpp"
#include "space_hst.hpp"
#include "space_obj.hpp"
#include "stdio.hpp"
#include "timer.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Ec::cache { sizeof (Ec_arch), Kobject::alignment };

Atomic<Ec *>    Ec::current     { nullptr };
Ec *            Ec::fpowner     { nullptr };
unsigned        Ec::donations   { 0 };

// Factory: Kernel Thread
Ec *Ec::create (cpu_t c, cont_t x)
{
    // Acquire references
    Refptr<Space_obj> ref_obj { &Space_obj::nova };
    Refptr<Space_hst> ref_hst { &Space_hst::nova };
    Refptr<Space_pio> ref_pio { nullptr };

    // Failed to acquire references
    if (EXPECT_FALSE (!ref_obj || !ref_hst))
        return nullptr;

    // Create EC
    auto const ec { new (cache) Ec_arch { ref_obj, ref_hst, ref_pio, c, x } };

    // If we created the EC, then references must have been consumed
    assert (!ec || (!ref_obj && !ref_hst && !ref_pio));

    return ec;
}

// Factory: HST EC
Ec *Ec::create_hst (Status &s, Pd *pd, bool t, bool fpu, cpu_t cpu, unsigned long evt, uintptr_t sp, uintptr_t hva)
{
    // Acquire references
    Refptr<Space_obj> ref_obj { pd->get_obj() };
    Refptr<Space_hst> ref_hst { pd->get_hst() };
    Refptr<Space_pio> ref_pio { pd->get_pio() };

    // Failed to acquire references
    if (EXPECT_FALSE (!ref_obj || !ref_hst || (Ec_arch::needs_pio && !ref_pio))) {
        s = Status::ABORTED;
        return nullptr;
    }

    auto const f { fpu ? new (pd->fpu_cache) Fpu : nullptr };
    auto const u { new Utcb };
    Ec *ec;

    if (EXPECT_TRUE ((!fpu || f) && u && (ec = new (cache) Ec_arch { t, f, ref_obj, ref_hst, ref_pio, cpu, evt, sp, hva, u }))) {
        assert (!ref_obj && !ref_hst && !ref_pio);
        return ec;
    }

    delete u;
    Fpu::operator delete (f, pd->fpu_cache);

    s = Status::MEM_OBJ;

    return nullptr;
}

void Ec::create_idle()
{
    Status s;

    auto const ec { Ec::create (Cpu::id, idle) };
    auto const sc { Pd::create_sc (s, &Space_obj::nova, Space_obj::Selector::NOVA_CPU + Cpu::id, ec, Cpu::id, 1000, 0, 0) };

    assert (ec && sc);

    current = ec;
    Scheduler::set_current (sc);
}

void Ec::create_root()
{
    trace (TRACE_PERF, "TIME: %lums %lums/%lums",
           Stc::ticks_to_ms (Timer::time() - Multiboot::t0),
           Stc::ticks_to_ms (Multiboot::t1 - Multiboot::t0),
           Stc::ticks_to_ms (Multiboot::t2 - Multiboot::t1));

    auto const ra { Multiboot::ra };

    if (EXPECT_FALSE (!ra)) {
        trace (TRACE_ROOT, "ROOT: No image");
        return;
    }

    Status s;

    Pd::root = Pd::create (s);
    Space_obj::nova.insert (Space_obj::Selector::ROOT_PD, Capability (Pd::root, std::to_underlying (Capability::Perm_pd::DEFINED)));

    auto const obj { Pd::root->create_obj (s, &Space_obj::nova, Space_obj::Selector::ROOT_OBJ) };
    auto const hst { Pd::root->create_hst (s, &Space_obj::nova, Space_obj::Selector::ROOT_HST) };
                     Pd::root->create_pio (s, &Space_obj::nova, Space_obj::Selector::ROOT_PIO);

    obj->delegate (&Space_obj::nova, Space_obj::Selector::NOVA_OBJ, Space_obj::selectors - 1, 0, Capability::pmask);
    obj->delegate (&Space_obj::nova, Space_obj::Selector::ROOT_OBJ, Space_obj::selectors - 2, 0, Capability::pmask);
    obj->delegate (&Space_obj::nova, Space_obj::Selector::ROOT_PD,  Space_obj::selectors - 3, 0, Capability::pmask);

    auto info_addr { (Space_hst::selectors() - 1) << PAGE_BITS };
    auto utcb_addr { (Space_hst::selectors() - 2) << PAGE_BITS };

    auto const ec { Pd::create_ec (s, obj, Space_obj::selectors - 4, Pd::root, Cpu::id, 0, 0, utcb_addr, BIT (2) | BIT (1)) };
    auto const sc { Pd::create_sc (s, obj, Space_obj::selectors - 5, ec, Cpu::id, 1000, Scheduler::priorities - 1, 0) };

    if (EXPECT_FALSE (!ec || !sc))
        return;

    auto const e { static_cast<Eh const *>(Hptp::map (MMAP_GLB_MAP0, ra)) };

    if (EXPECT_FALSE (!e->valid (ELF_MACHINE))) {
        trace (TRACE_ROOT, "ROOT: Invalid image");
        return;
    }

    auto const abi { Sys_abi (ec->sys_regs()) };

    abi.p0() = Multiboot::p0;
    abi.p1() = Multiboot::p1;
    abi.p2() = Multiboot::p2;

    ec->cont = Ec_arch::ret_user_hypercall;
    ec->exc_regs().ip() = e->entry;
    ec->exc_regs().sp() = info_addr;

    uint64_t root_e { 0 }, root_s { root_e - 1 };

    auto p { static_cast<Ph const *>(Hptp::map (MMAP_GLB_MAP1, ra + e->ph_offset)) };

    for (auto c { e->ph_count }; c--; p++) {

        if (p->type != 1 || !p->f_size || !p->flags)
            continue;

        if (p->f_size != p->m_size || p->v_addr % PAGE_SIZE (0) != (p->f_offs + ra) % PAGE_SIZE (0))
            return;

        auto perm { Paging::Permissions (Paging::R  * !!(p->flags & BIT (2)) |
                                         Paging::W  * !!(p->flags & BIT (1)) |
                                         Paging::XU * !!(p->flags & BIT (0)) |
                                         Paging::U) };

        trace (TRACE_ROOT | TRACE_PARSE, "ROOT: P:%#lx => V:%#lx S:%#10lx (%#x)", p->f_offs + ra, p->v_addr, p->f_size, perm);

        auto phys { align_dn (p->f_offs + ra, PAGE_SIZE (0)) };
        auto virt { align_dn (p->v_addr, PAGE_SIZE (0)) };
        auto size { align_up (p->v_addr + p->f_size, PAGE_SIZE (0)) - virt };

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
