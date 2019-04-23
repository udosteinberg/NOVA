/*
 * Execution Context (EC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

#include "counter.hpp"
#include "ec_arch.hpp"
#include "elf.hpp"
#include "extern.hpp"
#include "hazards.hpp"
#include "hip.hpp"
#include "interrupt.hpp"
#include "pd_kern.hpp"
#include "sm.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Ec::cache (sizeof (Ec), 32);

Ec *Ec::current { nullptr };
Ec *Ec::fpowner { nullptr };

// Kernel Thread
Ec::Ec (unsigned c, void (*x)()) : Kobject (Kobject::Type::EC, Kobject::Subtype::EC_GLOBAL), cpu (c), pd (&Pd_kern::nova()), cont (x) {}

void Ec::activate()
{
    Ec *ec = this;

    for (Sc::ctr_link = 0; ec->callee; ec = ec->callee)
        Sc::ctr_link++;

    if (EXPECT_TRUE (!ec->blocked() || !ec->block_sc()))
        static_cast<Ec_arch *>(ec)->make_current();
}

void Ec::help (void (*c)())
{
    if (EXPECT_FALSE (cont == dead))
        return;

    Counter::helping.inc();

    current->cont = c;

    if (EXPECT_FALSE (++Sc::ctr_loop >= 100))
        kill ("Livelock");

    activate();

    Sc::schedule (true);
}

void Ec::idle()
{
    trace (TRACE_CONT, "%s", __func__);

    for (;;) {

        auto hzd = Cpu::hazard & (HZD_RCU | HZD_SCHED);
        if (EXPECT_FALSE (hzd))
            handle_hazard (hzd, idle);

        Cpu::halt();
    }
}

void Ec::kill (char const *reason)
{
    trace (TRACE_KILL, "Killed EC:%p (%s)", static_cast<void *>(current), reason);

    Ec *ec = current->caller;

    if (ec)
        ec->cont = ec->cont == Ec_arch::ret_user_hypercall ? static_cast<void (*)()>(sys_finish<Status::ABORTED>) : dead;

    reply (dead);
}

void Ec::root_invoke()
{
    auto const root = root_addr();

    auto e = static_cast<Eh *>(Hpt::map (root));
    if (!root || !e->valid (Eh::ELF_CLASS, Eh::ELF_MACHINE))
        kill ("No ELF");

    constexpr auto hip_addr = (Space_mem::num - 1) << PAGE_BITS;

    current->regs.p0() = *reinterpret_cast<uintptr_t *>(Buddy::sym_to_virt (&__boot_p0));
    current->regs.p1() = *reinterpret_cast<uintptr_t *>(Buddy::sym_to_virt (&__boot_p1));
    current->regs.p2() = *reinterpret_cast<uintptr_t *>(Buddy::sym_to_virt (&__boot_p2));
    current->regs.sp() = hip_addr;
    current->regs.ip() = e->entry;

    auto c = ACCESS_ONCE (e->ph_count);
    auto p = static_cast<ELF_PHDR *>(Hpt::map (root + ACCESS_ONCE (e->ph_offset)));

    uint64 root_e = 0, root_s = root_e - 1;

    for (unsigned i = 0; i < c; i++, p++) {

        if (p->type == 1) {

            auto perm = Paging::Permissions (Paging::R * !!(p->flags & BIT (2)) |
                                             Paging::W * !!(p->flags & BIT (1)) |
                                             Paging::X * !!(p->flags & BIT (0)) |
                                             Paging::U);

            trace (TRACE_ROOT, "ROOT: P:%#llx => V:%#llx PM:%#x FS:%#llx MS:%#llx", p->f_offs + root, p->v_addr, perm, p->f_size, p->m_size);

            if (p->f_size != p->m_size || p->v_addr % PAGE_SIZE != (p->f_offs + root) % PAGE_SIZE)
                kill ("Bad ELF");

            uint64 phys = align_dn (p->f_offs + root, PAGE_SIZE);
            uint64 virt = align_dn (p->v_addr, PAGE_SIZE);
            uint64 size = align_up (p->v_addr + p->f_size, PAGE_SIZE) - virt;

            root_s = min (root_s, phys);
            root_e = max (root_e, phys + size);

            for (unsigned o; size; size -= BIT64 (o), phys += BIT64 (o), virt += BIT64 (o))
                current->pd->update_space_mem (&Pd_kern::nova(), phys >> PAGE_BITS, virt >> PAGE_BITS, (o = static_cast<unsigned>(min (max_order (phys, size), max_order (virt, size)))) - PAGE_BITS, perm, Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER, Space::Index::CPU_HST);
        }
    }

    for (unsigned i = 0; i < Interrupt::count(); i++)
        if (Interrupt::int_table[i].sm)
            Pd_kern::insert_user_obj (1024 + i, Capability (Interrupt::int_table[i].sm, BIT (4) | BIT (1)));
        else
            trace (TRACE_ROOT, "ROOT: INTR: %u unavailable", i);

    Hip::hip->build (root_s, root_e);
    current->pd->Space_mem::update (hip_addr, Buddy::ptr_to_phys (Hip::hip), 0, Paging::Permissions (Paging::K | Paging::U | Paging::R), Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);

    current->pd->Space_obj::insert (Space_obj::num - 1, Capability (&Pd_kern::nova(), BIT (0)));
    current->pd->Space_obj::insert (Space_obj::num - 2, Capability (current->pd, BIT_RANGE (4, 0)));
    current->pd->Space_obj::insert (Space_obj::num - 3, Capability (Ec::current, BIT_RANGE (4, 0)));
    current->pd->Space_obj::insert (Space_obj::num - 4, Capability (Sc::current, BIT_RANGE (4, 0)));

    Console::flush();

    Ec_arch::ret_user_hypercall();
}
