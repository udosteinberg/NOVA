/*
 * Read-Copy Update (RCU)
 *
 * Author: Udo Steinberg <udo@hypervisor.org>
 * TU Dresden, Operating Systems Group
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
#include "rcu.h"

unsigned    Rcu::batch;
unsigned    Rcu::count;

unsigned    Rcu::q_batch;
unsigned    Rcu::c_batch;
bool        Rcu::q_passed;
bool        Rcu::q_pending;

Rcu_elem *  Rcu::list_n;
Rcu_elem *  Rcu::list_c;
Rcu_elem *  Rcu::list_d;

void Rcu::invoke_batch()
{
    for (Rcu_elem *e = list_d, *next; e; e = next) {
        next = e->next;
        (e->func)(e);
    }
}

void Rcu::start_batch()
{
    count = Cpu::booted;

    batch++;
}

void Rcu::check_quiescent_state()
{
    if (q_batch != batch) {
        q_batch = batch;
        q_pending = true;
        q_passed = false;
        Counter::print (q_batch, Console_vga::COLOR_LIGHT_GREEN, 2);
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
    if (list_c && batch > c_batch) {
        list_d = list_c;
        list_c = 0;
    }

    if (!list_c && list_n) {
        list_c = list_n;
        list_n = 0;

        c_batch = batch + 1;
    }

    check_quiescent_state();

    if (list_d) {
        invoke_batch();
        list_d = 0;
    }

    return false;
}
