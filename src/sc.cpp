/*
 * Scheduling Context
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

#include "ec.h"
#include "initprio.h"
#include "lapic.h"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Sc::cache (sizeof (Sc), 8);

// Current SC
Sc *        Sc::current;
unsigned    Sc::ctr_link;
unsigned    Sc::ctr_loop;

// Ready List
Sc *Sc::list[Sc::priorities];

unsigned long Sc::prio_top;

// Release Queue
Sc::Rq Sc::release[NUM_CPU];

Sc::Sc (Pd *own, mword sel, Ec *o, mword c, mword p, mword q) : Kobject (own, sel, SC), owner (o), cpu (c), prio (p), full (Lapic::freq_bus / 1000 * q), left (0)
{
    trace (TRACE_SYSCALL, "SC:%p created (EC:%p CPU:%#lx P:%#lx Q:%#lx)", this, o, c, p, q);
}

void Sc::ready_enqueue()
{
    assert (cpu == Cpu::id);
    assert (this != reinterpret_cast<Sc *>(~0ul));

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

    trace (TRACE_SCHEDULE, "ENQ:%p (%02lu) PRIO:%#lx TOP:%#lx %s", this, left, prio, prio_top, prio > current->prio ? "reschedule" : "");

    if (prio > current->prio || (this != current && prio == current->prio && left))
        Cpu::hazard |= Cpu::HZD_SCHED;

    if (!left)
        left = full;
}

void Sc::ready_dequeue()
{
    assert (cpu == Cpu::id);
    assert (prev && next);

    if (list[prio] == this)
        list[prio] = next == this ? 0 : next;

    next->prev = prev;
    prev->next = next;
    prev = 0;

    while (!list[prio_top] && prio_top)
        prio_top--;

    trace (TRACE_SCHEDULE, "DEQ:%p (%02lu) PRIO:%#lx TOP:%#lx", this, left, prio, prio_top);
}

void Sc::schedule (bool suspend)
{
    Counter::count (Counter::schedule, Console_vga::COLOR_LIGHT_CYAN, 1);

    assert (current);
    assert (suspend || !current->prev);

    current->left = Lapic::get_timer();

    Cpu::hazard &= ~Cpu::HZD_SCHED;

    if (EXPECT_TRUE (!suspend))
        current->ready_enqueue();

    Sc *sc = list[prio_top];
    assert (sc);

    Lapic::set_timer (static_cast<uint32>(sc->left));

    current = sc;
    sc->ready_dequeue();

    ctr_loop = 0;

    Ec::activate (sc->owner);
}

void Sc::remote_enqueue()
{
    if (Cpu::id == cpu)
        ready_enqueue();

    else {
        Sc::Rq *rq = release + cpu;

        Lock_guard <Spinlock> guard (rq->lock);

        if (!rq->queue)
            rq->queue = prev = next = this;
        else {
            next = rq->queue;
            prev = rq->queue->prev;
            next->prev = prev->next = this;
        }

        Lapic::send_ipi (cpu, Lapic::DST_PHYSICAL, Lapic::DLV_FIXED, VEC_IPI_RRQ);
    }
}

void Sc::remote_enqueue_handler()
{
    Sc::Rq *rq = release + Cpu::id;

    Lock_guard <Spinlock> guard (rq->lock);

    for (Sc *ptr = rq->queue; ptr; ) {

        ptr->next->prev = ptr->prev;
        ptr->prev->next = ptr->next;

        Sc *sc = ptr;

        // Update ptr here, because ready_enqueue destroys prev/next
        ptr = ptr->next == ptr ? 0 : ptr->next;

        sc->ready_enqueue();
    }

    rq->queue = 0;
}
