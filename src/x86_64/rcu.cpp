/*
 * Read-Copy Update (RCU)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

        // Abort if another CPU has already started a new epoch
        if (e >> 2 != epoch_l)
            return;

        // Abort if another CPU has already set the state bit
        if (e & s)
            return;

    } while (!epoch.compare_exchange_n (e, e | s));

    // Start a new epoch only if we managed to set s and the other bit was already set
    if ((e ^ ~s) & State::FULL)
        return;

    // All CPUs must pass through a quiescent state
    count = Cpu::count;

    // Start new epoch with all state bits cleared
    epoch++;
}

/*
 * Report a quiescent state for this CPU in the current epoch
 */
void Rcu::quiet()
{
    // Quiescent state reporting must be active
    assert (Cpu::hazard & HZD_RCU);

    // Disable quiescent state reporting
    Cpu::hazard &= ~HZD_RCU;

    // The last CPU that passes through a quiescent state completes the epoch
    if (!--count) [[unlikely]]
        set_state (State::COMPLETED);
}

/*
 * Check RCU state and manage callback lifecycle
 */
void Rcu::check()
{
    Epoch e { epoch }, g { e >> 2 };

    // Check if a new epoch started
    if (epoch_l != g) {
        epoch_l = g;
        Cpu::hazard |= HZD_RCU;
    }

    // Check if the current callbacks have completed
    if (curr.head && complete (e, epoch_c))
        done.append (&curr);

    // Check if the next callbacks can be submitted
    if (next.head && !curr.head) {
        curr.append (&next);

        // Associate them with the next epoch
        epoch_c = g + 1;

        // Request the next epoch
        set_state (State::REQUESTED);
    }

    // Check if done callbacks can be invoked
    if (done.head)
        handle_callbacks();
}
