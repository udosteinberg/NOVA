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
Queue<Sc>   Sc::list[priorities];

unsigned Sc::prio_top;

Sc::Sc (Pd *, mword sel, Ec *e) : Kobject (Kobject::Type::SC), ec (e), cpu (static_cast<unsigned>(sel)), prio (0), budget (Stc::ms_to_ticks (1000)), left (0)
{
    trace (TRACE_SYSCALL, "SC:%p created (Kernel)", this);
}

Sc::Sc (Pd *, mword, Ec *e, unsigned c, unsigned p, unsigned q) : Kobject (Kobject::Type::SC), ec (e), cpu (c), prio (p), budget (Stc::ms_to_ticks (q)), left (0)
{
    trace (TRACE_SYSCALL, "SC:%p created (EC:%p CPU:%#x P:%#x Q:%#x)", this, e, c, p, q);
}

void Sc::ready_enqueue (uint64 t)
{
    assert (prio < priorities);
    assert (cpu == Cpu::id);

    if (prio > prio_top)
        prio_top = prio;

    list[prio].enqueue (this, left);

    if (prio > current->prio || (this != current && prio == current->prio && left))
        Cpu::hazard |= HZD_SCHED;

    if (!left)
        left = budget;

    tsc = t;
}

Sc * Sc::ready_dequeue (uint64 t)
{
    auto sc = list[prio_top].dequeue_head();

    assert (sc);
    assert (sc->prio < priorities);
    assert (sc->cpu == Cpu::id);

    while (list[prio_top].empty() && prio_top)
        prio_top--;

    sc->ec->add_tsc_offset (sc->tsc - t);

    sc->tsc = t;

    return sc;
}

void Sc::schedule (bool blocked)
{
    Counter::schedule.inc();

    assert (current);
    assert (blocked || !current->queued());

    uint64 t = rdtsc();
    uint64 d = Timeout_budget::timeout.dequeue();

    current->time += t - current->tsc;
    current->left = d > t ? d - t : 0;

    Cpu::hazard &= ~HZD_SCHED;

    if (EXPECT_TRUE (!blocked))
        current->ready_enqueue (t);

    ctr_loop = 0;
    current = ready_dequeue (t);

    Timeout_budget::timeout.enqueue (t + current->left);
    current->ec->activate();
}

void Sc::remote_enqueue()
{
    if (Cpu::id == cpu)
        return ready_enqueue (rdtsc());

    bool ipi;

    {
        auto r = remote (cpu);

        Lock_guard <Spinlock> guard (r->lock);

        ipi = r->queue.enqueue_tail (this);
    }

    if (ipi)
        Interrupt::send_cpu (Interrupt::Request::RRQ, cpu);
}

void Sc::rrq_handler()
{
    uint64 t = rdtsc();

    Lock_guard <Spinlock> guard (rq.lock);

    for (Sc *sc; (sc = rq.queue.dequeue_head()); sc->ready_enqueue (t)) ;
}
