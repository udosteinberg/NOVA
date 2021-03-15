/*
 * Scheduling Context
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "ec.hpp"
#include "interrupt.hpp"
#include "stc.hpp"
#include "stdio.hpp"
#include "timeout_budget.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Sc::cache (sizeof (Sc), 32);

INIT_PRIORITY (PRIO_LOCAL)
Sc::Rq Sc::rq;

Sc *        Sc::current;
unsigned    Sc::ctr_link;
unsigned    Sc::ctr_loop;

Sc *Sc::list[Sc::priorities];

unsigned Sc::prio_top;

Sc::Sc (Pd *, mword sel, Ec *e) : Kobject (Kobject::Type::SC), ec (e), cpu (static_cast<unsigned>(sel)), prio (0), budget (Stc::ms_to_ticks (1000)), left (0), prev (nullptr), next (nullptr)
{
    trace (TRACE_SYSCALL, "SC:%p created (Kernel)", this);
}

Sc::Sc (Pd *, mword, Ec *e, unsigned c, unsigned p, unsigned q) : Kobject (Kobject::Type::SC), ec (e), cpu (c), prio (p), budget (Stc::ms_to_ticks (q)), left (0), prev (nullptr), next (nullptr)
{
    trace (TRACE_SYSCALL, "SC:%p created (EC:%p CPU:%#x P:%#x Q:%#x)", this, e, c, p, q);
}

void Sc::ready_enqueue (uint64 t)
{
    assert (prio < priorities);
    assert (cpu == Cpu::id);

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

    trace (TRACE_SCHEDULE, "ENQ:%p (%llu) PRIO:%#x TOP:%#x %s", this, left, prio, prio_top, prio > current->prio ? "reschedule" : "");

    if (prio > current->prio || (this != current && prio == current->prio && left))
        Cpu::hazard |= HZD_SCHED;

    if (!left)
        left = budget;

    tsc = t;
}

void Sc::ready_dequeue (uint64 t)
{
    assert (prio < priorities);
    assert (cpu == Cpu::id);
    assert (prev && next);

    if (list[prio] == this)
        list[prio] = next == this ? nullptr : next;

    next->prev = prev;
    prev->next = next;
    prev = next = nullptr;

    while (!list[prio_top] && prio_top)
        prio_top--;

    trace (TRACE_SCHEDULE, "DEQ:%p (%llu) PRIO:%#x TOP:%#x", this, left, prio, prio_top);

    ec->add_tsc_offset (tsc - t);

    tsc = t;
}

void Sc::schedule (bool suspend)
{
    Counter::schedule.inc();

    assert (current);
    assert (suspend || !current->prev);

    uint64 t = rdtsc();
    uint64 d = Timeout_budget::budget.dequeue();

    current->time += t - current->tsc;
    current->left = d > t ? d - t : 0;

    Cpu::hazard &= ~HZD_SCHED;

    if (EXPECT_TRUE (!suspend))
        current->ready_enqueue (t);

    Sc *sc = list[prio_top];
    assert (sc);

    Timeout_budget::budget.enqueue (t + sc->left);

    ctr_loop = 0;

    current = sc;
    sc->ready_dequeue (t);
    sc->ec->activate();
}

void Sc::remote_enqueue()
{
    if (Cpu::id == cpu)
        ready_enqueue (rdtsc());

    else {
        Sc::Rq *r = remote (cpu);

        Lock_guard <Spinlock> guard (r->lock);

        if (r->queue) {
            next = r->queue;
            prev = r->queue->prev;
            next->prev = prev->next = this;
        } else {
            r->queue = prev = next = this;
            Interrupt::send_cpu (Interrupt::Request::RRQ, cpu);
        }
    }
}

void Sc::rrq_handler()
{
    uint64 t = rdtsc();

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
