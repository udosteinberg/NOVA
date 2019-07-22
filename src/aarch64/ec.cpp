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

#include "ec_arch.hpp"
#include "hazards.hpp"
#include "pd_kern.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Ec::cache (sizeof (Ec), Kobject::alignment);

Ec *Ec::current { nullptr };
Ec *Ec::fpowner { nullptr };

// Kernel Thread
Ec::Ec (unsigned c, void (*x)()) : Kobject (Kobject::Type::EC, Kobject::Subtype::EC_GLOBAL), cpu (c), pd (&Pd_kern::nova()), cont (x) {}

void Ec::activate()
{
    Ec *ec = this;

    if (EXPECT_TRUE (!ec->blocked() || !ec->block_sc()))
        static_cast<Ec_arch *>(ec)->make_current();
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

    // XXX: FIXME
    for (;;)
        Cpu::halt();
}
