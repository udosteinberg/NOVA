/*
 * Scheduling Context (SC)
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

#include "assert.hpp"
#include "counter.hpp"
#include "ec.hpp"
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
Sc *        Sc::list[priorities]    { nullptr };

Sc::Sc (unsigned c, Ec *e, unsigned p, unsigned b) : Kobject (Kobject::Type::SC), cpu (c), ec (e), prio (p), budget (Timer::ms_to_ticks (b))
{
    trace (TRACE_CREATE, "SC:%p created - CPU:%u EC:%p", static_cast<void *>(this), cpu, static_cast<void *>(ec));
}

void Sc::ready_enqueue (uint64 t)
{
    assert (cpu == Cpu::id);
    assert (prio < priorities);

    if (prio > prio_top)
        prio_top = prio;

    if (!list[prio])
        list[prio] = prev = next = this;
    else {
        next = list[prio];
        prev = list[prio]->prev;
        next->prev = prev->next = this;
        if (left)
            list[prio] = this;
    }

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
    assert (prev && next);

    if (list[prio] == this)
        list[prio] = next == this ? nullptr : next;

    next->prev = prev;
    prev->next = next;
    prev = next = nullptr;

    while (!list[prio_top] && prio_top)
        prio_top--;

    ec->adjust_offset_ticks (t - last);

    last = t;
}

void Sc::schedule (bool suspend)
{
    Counter::schedule.inc();

    assert (current);
    assert (suspend || (!current->prev && !current->next));

    uint64 t = Timer::time();
    uint64 d = Timeout_budget::timeout.dequeue();

    current->time += t - current->last;
    current->left = d > t ? d - t : 0;

    Cpu::hazard &= ~HZD_SCHED;

    if (EXPECT_TRUE (!suspend))
        current->ready_enqueue (t);

    for (Sc *sc; (current = sc = list[prio_top]); ) {

        ctr_loop = 0;

        assert (sc);
        sc->ready_dequeue (t);

        Timeout_budget::timeout.enqueue (t + sc->left);
        sc->ec->activate();
        Timeout_budget::timeout.dequeue();
    }

    UNREACHED;
}

void Sc::remote_enqueue()
{
    if (Cpu::id == cpu)
        ready_enqueue (Timer::time());

    else {
        auto r = remote_queue (cpu);

        Lock_guard <Spinlock> guard (r->lock);

        if (!r->queue) {
            r->queue = prev = next = this;
            Interrupt::send_cpu (cpu, Interrupt::Request::RRQ);
        } else {
            next = r->queue;
            prev = r->queue->prev;
            next->prev = prev->next = this;
        }
    }
}

void Sc::rrq_handler()
{
    uint64 t = Timer::time();

    Lock_guard <Spinlock> guard (rq.lock);

    for (Sc *ptr = rq.queue; ptr; ) {

        ptr->next->prev = ptr->prev;
        ptr->prev->next = ptr->next;

        Sc *sc = ptr;

        ptr = ptr->next == ptr ? nullptr : ptr->next;

        sc->ready_enqueue (t);
    }

    rq.queue = nullptr;
}
