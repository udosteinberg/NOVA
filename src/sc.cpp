/*
 * Scheduling Context (SC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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
#include "cos.hpp"
#include "counter.hpp"
#include "ec.hpp"
#include "interrupt.hpp"
#include "stdio.hpp"
#include "timeout_budget.hpp"
#include "timer.hpp"

INIT_PRIORITY (PRIO_SLAB)   Slab_cache Sc::cache (sizeof (Sc), Kobject::alignment);
INIT_PRIORITY (PRIO_LOCAL)  Scheduler::Ready    Scheduler::ready;
INIT_PRIORITY (PRIO_LOCAL)  Scheduler::Release  Scheduler::release;

Sc *Scheduler::current { nullptr };

Sc::Sc (unsigned n, Ec *e, uint16 b, uint8 p, uint16 c) : Kobject (Kobject::Type::SC), ec (e), budget (Stc::ms_to_ticks (b)), cpu (n), cos (c), prio (p)
{
    trace (TRACE_CREATE, "SC:%p created (EC:%p CPU:%u Budget:%ums Prio:%u COS:%u)", static_cast<void *>(this), static_cast<void *>(ec), cpu, b, p, c);
}

void Scheduler::Ready::enqueue (Sc *sc, uint64 t)
{
    assert (sc->cpu == Cpu::id);
    assert (sc->prio < priorities);

    if (sc->prio > prio_top)
        prio_top = sc->prio;

    queue[sc->prio].enqueue (sc, sc->left);

    if (sc->prio > current->prio || (sc != current && sc->prio == current->prio && sc->left))
        Cpu::hazard |= Hazard::SCHED;

    if (!sc->left)
        sc->left = sc->budget;

    sc->last = t;
}

auto Scheduler::Ready::dequeue (uint64 t)
{
    auto const sc { queue[prio_top].dequeue_head() };

    assert (sc);
    assert (sc->cpu == Cpu::id);
    assert (sc->prio < priorities);

    while (queue[prio_top].empty() && prio_top)
        prio_top--;

    if (EXPECT_TRUE (sc->ec != current->ec))
        sc->ec->adjust_offset_ticks (t - sc->last);

    sc->last = t;

    return sc;
}

void Scheduler::Release::enqueue (Sc *sc)
{
    auto const r { Kmem::loc_to_glob (this, sc->cpu) };

    bool notify;

    {   Lock_guard <Spinlock> guard (r->lock);

        notify = r->queue.enqueue_tail (sc);
    }

    if (notify)
        Interrupt::send_cpu (Interrupt::Request::RRQ, sc->cpu);
}

auto Scheduler::Release::dequeue()
{
    Lock_guard <Spinlock> guard (lock);

    return queue.dequeue_head();
}

void Scheduler::unblock (Sc *sc)
{
    if (Cpu::id == sc->cpu)
        ready.enqueue (sc, Timer::time());
    else
        release.enqueue (sc);
}

void Scheduler::requeue()
{
    auto const t { Timer::time() };

    for (Sc *sc; (sc = release.dequeue()); ready.enqueue (sc, t)) ;
}

void Scheduler::schedule (bool blocked)
{
    Counter::schedule.inc();

    assert (current);
    assert (blocked || !current->queued());

    auto const t { Timer::time() };
    auto const d { Timeout_budget::timeout.dequeue() };

    current->used = current->used + (t - current->last);
    current->left = d > t ? d - t : 0;

    Cpu::hazard &= ~Hazard::SCHED;

    if (EXPECT_TRUE (!blocked))
        ready.enqueue (current, t);

    for (;;) {

        current = ready.dequeue (t);

        Cos::make_current (current->cos);

        Timeout_budget::timeout.enqueue (t + current->left);
        current->ec->activate();
        Timeout_budget::timeout.dequeue();
    }
}
