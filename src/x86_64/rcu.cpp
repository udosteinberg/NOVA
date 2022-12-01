/*
 * Read-Copy Update (RCU)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "assert.hpp"
#include "counter.hpp"
#include "cpu.hpp"
#include "hazards.hpp"
#include "initprio.hpp"
#include "rcu.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_LOCAL) Rcu::List Rcu::next;
INIT_PRIORITY (PRIO_LOCAL) Rcu::List Rcu::curr;
INIT_PRIORITY (PRIO_LOCAL) Rcu::List Rcu::done;

Rcu::Epoch Rcu::epoch_l { 0 };
Rcu::Epoch Rcu::epoch_c { 0 };

void Rcu::handle_callbacks()
{
    for (Rcu_elem *e = done.head, *n; e; e = n) {
        n = e->next;
        (e->func)(e);
    }

    done.clear();
}

void Rcu::set_state (State s)
{
    Epoch e { epoch };

    do {

        if (e >> 2 != epoch_l)
            return;

        if (e & s)
            return;

    } while (!epoch.compare_exchange_n (e, e | s));

    if ((e ^ ~s) & State::FULL)
        return;

    count = Cpu::online;

    epoch++;
}

/*
 * Report a quiescent state for this CPU in the current epoch
 */
void Rcu::quiet()
{
    assert (Cpu::hazard & HZD_RCU);

    Cpu::hazard &= ~HZD_RCU;

    if (EXPECT_FALSE (!--count))
        set_state (State::COMPLETED);
}

/*
 * Check RCU state and manage callback lifecycle
 */
void Rcu::check()
{
    Epoch e { epoch }, g { e >> 2 };

    if (epoch_l != g) {
        epoch_l = g;
        Cpu::hazard |= HZD_RCU;
    }

    if (curr.head && complete (e, epoch_c))
        done.append (&curr);

    if (next.head && !curr.head) {
        curr.append (&next);

        epoch_c = g + 1;

        set_state (State::REQUESTED);
    }

    if (done.head)
        handle_callbacks();
}
