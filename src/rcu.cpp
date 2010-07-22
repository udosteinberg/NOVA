/*
 * Read-Copy Update (RCU)
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "assert.h"
#include "atomic.h"
#include "counter.h"
#include "cpu.h"
#include "initprio.h"
#include "rcu.h"

unsigned    Rcu::batch;
unsigned    Rcu::count;

unsigned    Rcu::q_batch;
unsigned    Rcu::c_batch;
bool        Rcu::q_passed;
bool        Rcu::q_pending;

INIT_PRIORITY (PRIO_LOCAL) Rcu_list Rcu::next;
INIT_PRIORITY (PRIO_LOCAL) Rcu_list Rcu::curr;
INIT_PRIORITY (PRIO_LOCAL) Rcu_list Rcu::done;

void Rcu::invoke_batch()
{
    for (Rcu_elem *e = done.head, *next; e; e = next) {
        next = e->next;
        (e->func)(e);
    }

    done.clear();
}

void Rcu::start_batch()
{
    count = Cpu::online;

    batch++;
}

void Rcu::check_quiescent_state()
{
    if (q_batch != batch) {
        q_batch = batch;
        q_pending = true;
        q_passed = false;
        Counter::print (q_batch, Console_vga::COLOR_LIGHT_GREEN, SPN_RCU);
        return;
    }

    if (!q_pending || !q_passed)
        return;

    q_pending = false;

    assert (q_batch == batch);

    if (Atomic::sub (count, 1U))
        start_batch();
}

bool Rcu::process_callbacks()
{
    if (curr.head && batch > c_batch)
        done.append (&curr);

    if (!curr.head && next.head) {
        curr.append (&next);

        c_batch = batch + 1;
    }

    check_quiescent_state();

    if (done.head)
        invoke_batch();

    return false;
}
