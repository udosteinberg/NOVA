/*
 * Execution Context (EC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "config.hpp"
#include "cpu.hpp"
#include "ec.hpp"
#include "event.hpp"
#include "extern.hpp"
#include "hazards.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Ec::cache (sizeof (Ec), 32);

Ec *Ec::current, *Ec::fpowner;

// Kernel Thread
Ec::Ec (unsigned c, void (*x)()) : Kobject (Kobject::Type::EC, Kobject::Subtype::EC_GLOBAL), cpu (c), pd (&Pd::kern), cont (x) {}

// Hyperthread
Ec::Ec (Pd *p, Fpu *f, Utcb *u, unsigned c, mword e, mword a, mword s, void (*x)()) : Kobject (Kobject::Type::EC, x ? Kobject::Subtype::EC_GLOBAL : Kobject::Subtype::EC_LOCAL), cpu (c), evt (e), pd (p), fpu (f), utcb (u), cont (x)
{
    trace (TRACE_CREATE, "EC:%p created - CPU:%u PD:%p %c", static_cast<void *>(this), cpu, static_cast<void *>(pd), subtype == Kobject::Subtype::EC_LOCAL ? 'L' : 'G');

    regs.set_sp (s);
    regs.set_ep (Event::hst_arch + Event::Selector::STARTUP);

    pd->Space_mem::update (a, Buddy::ptr_to_phys (utcb), 0, Paging::Permissions (Paging::R | Paging::W | Paging::U), Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);
}

// vCPU
Ec::Ec (Pd *p, Fpu *f, Vmcb *v, unsigned c, mword e, void (*x)()) : Kobject (Kobject::Type::EC, Kobject::Subtype::EC_VCPU), cpu (c), evt (e), pd (p), fpu (f), vmcb (v), cont (x)
{
    trace (TRACE_CREATE, "EC:%p created - CPU:%u PD:%p %c", static_cast<void *>(this), cpu, static_cast<void *>(pd), 'V');

    regs.set_ep (Event::gst_arch + Event::Selector::STARTUP);
}

void Ec::activate()
{
    Ec *ec = this;

    for (Sc::ctr_link = 0; ec->callee; ec = ec->callee)
        Sc::ctr_link++;

    if (EXPECT_TRUE (!ec->blocked() || !ec->block_sc()))
        ec->make_current();
}

void Ec::help (void (*c)())
{
    if (EXPECT_FALSE (cont == dead))
        return;

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
        ec->cont = ec->cont == ret_user_hypercall ? static_cast<void (*)()>(sys_finish<Sys_regs::ABORTED>) : dead;

    reply (dead);
}
