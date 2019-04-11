/*
 * Scheduling Context (SC)
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

#include "assert.hpp"
#include "counter.hpp"
#include "hazards.hpp"
#include "interrupt.hpp"
#include "lock_guard.hpp"
#include "sc.hpp"
#include "stdio.hpp"
#include "timeout_budget.hpp"
#include "timer.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Sc::cache (sizeof (Sc), Kobject::alignment);

INIT_PRIORITY (PRIO_LOCAL)
Sc::Rq Sc::rq;

unsigned    Sc::ctr_link            { 0 };
unsigned    Sc::ctr_loop            { 0 };
unsigned    Sc::prio_top            { 0 };
Sc *        Sc::current             { nullptr };
Queue<Sc>   Sc::list[priorities];

Sc::Sc (unsigned c, Ec *e, unsigned p, uint32 b) : Kobject (Kobject::Type::SC), cpu (c), ec (e), prio (p), budget (Timer::ms_to_ticks (b))
{
    trace (TRACE_CREATE, "SC:%p created - CPU:%u EC:%p", static_cast<void *>(this), cpu, static_cast<void *>(ec));
}

void Sc::ready_enqueue (uint64 t)
{
    assert (cpu == Cpu::id);
    assert (prio < priorities);

    if (prio > prio_top)
        prio_top = prio;

    list[prio].enqueue (this, left);

    if (prio > current->prio || (this != current && prio == current->prio && left))
        Cpu::hazard |= HZD_SCHED;

    if (!left)
        left = budget;

    last = t;
}

void Sc::ready_dequeue (uint64 t)
{
    assert (cpu == Cpu::id);
    assert (prio < priorities);
    assert (queued());

    list[prio].dequeue (this);

    while (!list[prio_top].head() && prio_top)
        prio_top--;

    last = t;
}

void Sc::schedule (bool blocked)
{
    Counter::schedule.inc();

    assert (current);
    assert (blocked || !current->queued());

    uint64 t = Timer::time();
    uint64 d = Timeout_budget::timeout.dequeue();

    current->used = current->used + (t - current->last);
    current->left = d > t ? d - t : 0;

    Cpu::hazard &= ~HZD_SCHED;

    if (EXPECT_TRUE (!blocked))
        current->ready_enqueue (t);

    while ((current = list[prio_top].head())) {

        ctr_loop = 0;

        current->ready_dequeue (t);

        Timeout_budget::timeout.enqueue (t + current->left);
        Timeout_budget::timeout.dequeue();
    }

    UNREACHED;
}

void Sc::remote_enqueue()
{
    if (Cpu::id == cpu)
        return ready_enqueue (Timer::time());

    bool ipi;

    {
        auto r = remote_queue (cpu);

        Lock_guard <Spinlock> guard (r->lock);

        ipi = r->queue.enqueue (this);
    }

    if (ipi)
        Interrupt::send_cpu (cpu, Interrupt::Request::RRQ);
}

void Sc::rrq_handler()
{
    uint64 t = Timer::time();

    Lock_guard <Spinlock> guard (rq.lock);

    for (Sc *sc; (sc = rq.queue.head()); sc->ready_enqueue (t))
        rq.queue.dequeue (sc);
}
