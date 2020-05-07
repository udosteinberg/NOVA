/*
 * Execution Context (EC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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
#include "hazards.hpp"
#include "pd_kern.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Ec::cache (sizeof (Ec), Kobject::alignment);

Atomic<Ec *>Ec::current { nullptr };
Ec *        Ec::fpowner { nullptr };

// Kernel Thread
Ec::Ec (unsigned c, cont_t x) : Kobject (Kobject::Type::EC, Kobject::Subtype::EC_GLOBAL), cpu (c), pd (&Pd_kern::nova()), cont (x) {}

void Ec::create_idle()
{
    Ec::current = Ec::create (Cpu::id, idle);
    Sc::current = Sc::create (Cpu::id, current, 0, 1000);
}

void Ec::activate()
{
    Ec *ec = this;

    for (Sc::ctr_link = 0; ec->callee; ec = ec->callee)
        Sc::ctr_link++;

    if (EXPECT_TRUE (!ec->blocked() || !ec->block_sc()))
        static_cast<Ec_arch *>(ec)->make_current();
}

void Ec::help (Ec *ec, cont_t c)
{
    if (EXPECT_FALSE (ec->cont == dead))
        return;

    Counter::helping.inc();

    cont = c;

    if (EXPECT_FALSE (++Sc::ctr_loop >= 100))
        ec->kill ("Livelock");

    ec->activate();

    Sc::schedule (true);
}

void Ec::idle (Ec *const self)
{
    trace (TRACE_CONT, "%s", __func__);

    for (;;) {

        auto hzd = Cpu::hazard & (HZD_RCU | HZD_SCHED);
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
