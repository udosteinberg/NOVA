/*
 * Read-Copy Update (RCU)
 *
 * Copyright (C) 2008-2010, Udo Steinberg <udo@hypervisor.org>
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

#include "atomic.h"
#include "cpu.h"
#include "lock_guard.h"
#include "rcu.h"

Spinlock    Rcu::g_lock;
unsigned    Rcu::g_cpumask;
unsigned    Rcu::g_current;
unsigned    Rcu::g_completed;
bool        Rcu::g_next_pending;

unsigned    Rcu::batch;
unsigned    Rcu::q_batch;
bool        Rcu::q_passed;
bool        Rcu::q_pending;
Rcu_elem *  Rcu::list_c;
Rcu_elem ** Rcu::tail_c;
Rcu_elem *  Rcu::list_n;
Rcu_elem ** Rcu::tail_n;
Rcu_elem *  Rcu::list_d;
Rcu_elem ** Rcu::tail_d;

void Rcu::init()
{
    q_pending = false;
    list_c = list_n = list_d = 0;
    tail_c = &list_c;
    tail_n = &list_n;
    tail_d = &list_d;
}

void Rcu::call (Rcu_elem *e, void (*f)(Rcu_elem *))
{
    e->func = f;
    e->next = 0;
   *tail_n  = e;
    tail_n  = &e->next;
}

void Rcu::quiet()
{
    Atomic::clr_mask<true>(g_cpumask, 1UL << Cpu::id);

    if (!g_cpumask) {
        g_completed = g_current;
        start_batch();
    }
}

void Rcu::start_batch()
{
    if (g_next_pending && g_completed == g_current) {
        g_next_pending = false;
        g_current++;
        g_cpumask = ~0UL;
    }
}

bool Rcu::do_batch()
{
    for (Rcu_elem *e = list_d; e; e = e->next)
        (e->func)(e);

    list_d = 0;
    tail_d = &list_d;

    return true;
}

void Rcu::check_quiescent_state()
{
    if (q_batch != g_current) {
        q_pending = true;
        q_passed = false;
        q_batch = g_current;
        return;
    }

    if (!q_pending)
        return;

    if (!q_passed)
        return;

    q_pending = false;

    Lock_guard<Spinlock> guard (g_lock);

    if (q_batch == g_current)
        quiet();
}

bool Rcu::pending()
{
    if (list_c && g_completed >= batch)
        return true;

    if (!list_c && list_n)
        return true;

    if (list_d)
        return true;

    if (q_batch != g_current || q_pending)
        return 1;

    return false;
}

bool Rcu::process_callbacks()
{
    if (list_c && g_completed >= batch) {
       *tail_d = list_c;
        tail_d = tail_c;
        list_c = 0;
        tail_c = &list_c;
    }

    if (!list_c && list_n) {
        list_c = list_n;
        tail_c = tail_n;
        list_n = 0;
        tail_n = &list_n;

        batch = g_current + 1;

        if (!g_next_pending) {
            Lock_guard<Spinlock> guard (g_lock);
            g_next_pending = true;
            start_batch();
        }
    }

    check_quiescent_state();

    if (list_d)
        do_batch();

    return false;
}
