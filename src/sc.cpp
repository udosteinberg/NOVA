/*
 * Scheduling Context
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
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

#include "ec.h"
#include "initprio.h"
#include "lapic.h"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Sc::cache (sizeof (Sc), 32);

INIT_PRIORITY (PRIO_LOCAL)
Sc::Rq Sc::rq;

Sc *        Sc::current;
unsigned    Sc::ctr_link;
unsigned    Sc::ctr_loop;

Sc *Sc::list[Sc::priorities];

unsigned Sc::prio_top;

Sc::Sc (Pd *own, mword sel, mword prm, Ec *e, unsigned c, unsigned p, unsigned q) : Kobject (SC, own, sel, prm), ec (e), cpu (c), prio (p), budget (Lapic::freq_bus / 1000 * q), left (0)
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

    trace (TRACE_SCHEDULE, "ENQ:%p (%02u) PRIO:%#x TOP:%#x %s", this, left, prio, prio_top, prio > current->prio ? "reschedule" : "");

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
        list[prio] = next == this ? 0 : next;

    next->prev = prev;
    prev->next = next;
    prev = 0;

    while (!list[prio_top] && prio_top)
        prio_top--;

    trace (TRACE_SCHEDULE, "DEQ:%p (%02u) PRIO:%#x TOP:%#x", this, left, prio, prio_top);

    ec->add_tsc_offset (tsc - t);

    tsc = t;
}

void Sc::schedule (bool suspend)
{
    Counter::print (++Counter::schedule, Console_vga::COLOR_LIGHT_CYAN, SPN_SCH);

    assert (current);
    assert (suspend || !current->prev);

    uint64 t = rdtsc();
    current->time += t - current->tsc;
    current->left = Lapic::get_timer();

    Cpu::hazard &= ~HZD_SCHED;

    if (EXPECT_TRUE (!suspend))
        current->ready_enqueue (t);

    Sc *sc = list[prio_top];
    assert (sc);

    Lapic::set_timer (static_cast<uint32>(sc->left));

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
            Lapic::send_ipi (cpu, Lapic::DLV_FIXED, VEC_IPI_RRQ);
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

        ptr = ptr->next == ptr ? 0 : ptr->next;

        sc->ready_enqueue (t);
    }

    rq.queue = 0;
}

void Sc::rke_handler()
{
    if (Pd::current->Space_mem::htlb.chk (Cpu::id) || Pd::current->Space_mem::gtlb.chk (Cpu::id))
        Cpu::hazard |= HZD_SCHED;
}
