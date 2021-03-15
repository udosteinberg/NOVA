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
#include "hazards.hpp"
#include "hip.hpp"
#include "interrupt.hpp"
#include "pd_kern.hpp"
#include "sm.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Ec::cache (sizeof (Ec_arch), Kobject::alignment);

Atomic<Ec *>    Ec::current     { nullptr };
Ec *            Ec::fpowner     { nullptr };
unsigned        Ec::donations   { 0 };

// Factory: Kernel Thread
Ec *Ec::create (unsigned c, cont_t x)
{
    return new Ec_arch (&Pd_kern::nova(), c, x);
}

// Factory: User Thread
Ec *Ec::create (Pd *p, bool fpu, unsigned c, unsigned long e, uintptr_t a, uintptr_t s, cont_t x)
{
    Ec *ec;
    auto f = fpu ? new (p->fpu_cache) Fpu : nullptr;
    auto u = new Utcb;

    if (EXPECT_TRUE ((!fpu || f) && u && (ec = new Ec_arch (p, f, u, c, e, a, s, x))))
        return ec;

    delete u;
    Fpu::operator delete (f, p->fpu_cache);

    return nullptr;
}

void Ec::create_idle()
{
    current = Ec::create (Cpu::id, idle);
    Sc::current = new Sc (&Pd_kern::nova(), Cpu::id, Ec::current);
}

void Ec::create_root()
{
    Pd::root = Pd::create();
    auto ec  = Ec::create (Pd::root, true, Cpu::id, 0, (Space_mem::num - 2) << PAGE_BITS, 0, root_invoke);
    auto sc  = new Sc (Pd::root, NUM_EXC + 2, ec, Cpu::id, Sc::default_prio, Sc::default_quantum);

    sc->remote_enqueue();
}

void Ec::activate()
{
    Ec *ec = this;

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
    if (EXPECT_FALSE (Cpu::hazard & HZD_SCHED))
        Sc::schedule (false);

    Counter::helping.inc();

    ec->activate();

    Sc::schedule (true);
}

void Ec::idle (Ec *const self)
{
    trace (TRACE_CONT, "%s", __func__);

    for (;;) {

        auto hzd = Cpu::hazard & (HZD_RCU | HZD_SLEEP | HZD_SCHED);
        if (EXPECT_FALSE (hzd))
            self->handle_hazard (hzd, idle);

        Cpu::halt();
    }
}

void Ec::kill (char const *reason)
{
    trace (TRACE_KILL, "Killed EC:%p (%s)", static_cast<void *>(this), reason);

    Ec *ec = caller;

    if (ec)
        ec->cont = ec->cont == Ec_arch::ret_user_hypercall ? sys_finish<Status::ABORTED> : dead;

    reply (dead);
}

void Ec::root_invoke (Ec *const self)
{
    auto const root = *reinterpret_cast<uintptr_t *>(Kmem::sym_to_virt (&__boot_ra));

    Eh const *e;
    if (!root || !(e = static_cast<Eh const *>(Hptp::map (root)))->valid (Eh::ELF_MACHINE))
        self->kill ("No ELF");

    constexpr auto hip_addr { (Space_mem::num - 1) << PAGE_BITS };

    auto abi { Sys_abi (self->sys_regs()) };
    abi.p0() = *reinterpret_cast<uintptr_t *>(Kmem::sym_to_virt (&__boot_p0));
    abi.p1() = *reinterpret_cast<uintptr_t *>(Kmem::sym_to_virt (&__boot_p1));
    abi.p2() = *reinterpret_cast<uintptr_t *>(Kmem::sym_to_virt (&__boot_p2));

    self->exc_regs().ip() = e->entry;
    self->exc_regs().sp() = hip_addr;

    auto c = __atomic_load_n (&e->ph_count, __ATOMIC_RELAXED);
    auto p = static_cast<Ph const *>(Hptp::map (root + __atomic_load_n (&e->ph_offset, __ATOMIC_RELAXED)));

    for (unsigned i = 0; i < c; i++, p++) {

        if (p->type == 1) {

            auto perm = Paging::Permissions (Paging::R  * !!(p->flags & BIT (2)) |
                                             Paging::W  * !!(p->flags & BIT (1)) |
                                             Paging::XU * !!(p->flags & BIT (0)) |
                                             Paging::U);

            if (p->f_size != p->m_size || p->v_addr % PAGE_SIZE != (p->f_offs + root) % PAGE_SIZE)
                self->kill ("Bad ELF");

            uint64 phys = align_dn (p->f_offs + root, PAGE_SIZE);
            uint64 virt = align_dn (p->v_addr, PAGE_SIZE);
            uint64 size = align_up (p->v_addr + p->f_size, PAGE_SIZE) - virt;

            for (unsigned o; size; size -= BIT64 (o), phys += BIT64 (o), virt += BIT64 (o))
                self->pd->delegate_mem (&Pd_kern::nova(), phys >> PAGE_BITS, virt >> PAGE_BITS, (o = static_cast<unsigned>(min (max_order (phys, size), max_order (virt, size)))) - PAGE_BITS, perm, Space::Index::CPU_HST, Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);
        }
    }

    for (unsigned i = 0; i < sizeof (Interrupt::int_table) / sizeof (*Interrupt::int_table); i++)
        if (Interrupt::int_table[i].sm)
            Pd_kern::insert_user_obj (1024 + i, Capability (Interrupt::int_table[i].sm, static_cast<unsigned>(Capability::Perm_sm::DEFINED_INT)));

    self->pd->Space_mem::update (hip_addr, Kmem::ptr_to_phys (Hip::hip), 0, Paging::Permissions (Paging::K | Paging::U | Paging::R), Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);

    self->pd->Space_obj::insert (Space_obj::num - 1, Capability (&Pd_kern::nova(),          static_cast<unsigned>(Capability::Perm_pd::CTRL)));
    self->pd->Space_obj::insert (Space_obj::num - 2, Capability (self->pd,                  static_cast<unsigned>(Capability::Perm_pd::DEFINED)));
    self->pd->Space_obj::insert (Space_obj::num - 3, Capability (self,                      static_cast<unsigned>(Capability::Perm_ec::DEFINED)));
    self->pd->Space_obj::insert (Space_obj::num - 4, Capability (Sc::current,               static_cast<unsigned>(Capability::Perm_sc::DEFINED)));

    Console::flush();

    Ec_arch::ret_user_hypercall (self);
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
        assert (!(Cpu::hazard & HZD_FPU));

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
