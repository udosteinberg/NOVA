/*
 * Memory Space
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

#include "bits.h"
#include "counter.h"
#include "hip.h"
#include "lapic.h"
#include "mtrr.h"
#include "pd.h"

unsigned Space_mem::did_ctr;

void Space_mem::init (unsigned cpu)
{
    if (exist.set (cpu)) {

        // Create ptab for this CPU
        percpu[cpu] = new Ptab;

        // Sync cpu-local range
        percpu[cpu]->sync_from (Pd::kern.cpu_ptab (cpu), LOCAL_SADDR);

        // Sync kernel code and data
        percpu[cpu]->sync_master_range (LINK_ADDR, LOCAL_SADDR);

        trace (TRACE_MEMORY, "PD:%p PTAB[%u]:%#lx", static_cast<Pd *>(this), cpu, Buddy::ptr_to_phys (percpu[cpu]));
    }
}

void Space_mem::update (Mdb *mdb, Paddr phys, mword rem)
{
    assert (this == mdb->node_pd && this != &Pd::kern);

    Lock_guard <Spinlock> guard (mdb->node_lock);

    mword o = mdb->node_order - PAGE_BITS;
    mword a = mdb->node_attr & ~rem;

    if (dpt) {
        unsigned ord = min (o, Dpt::ord);
        for (unsigned long i = 0; i < 1UL << (o - ord); i++)
            dpt->update (mdb->node_base + i * (1UL << (Dpt::ord + PAGE_BITS)), ord, phys + i * (1UL << (Dpt::ord + PAGE_BITS)), a, rem);
    }

    if (ept) {
        unsigned ord = min (o, Ept::ord);
        for (unsigned long i = 0; i < 1UL << (o - ord); i++)
            ept->update (mdb->node_base + i * (1UL << (Ept::ord + PAGE_BITS)), ord, phys + i * (1UL << (Ept::ord + PAGE_BITS)), Ept::hw_attr (a), mdb->node_type, rem);
    }

    bool l = mst->update (mdb->node_base, o, phys, Ptab::hw_attr (a), rem);

    if (rem) {

        if (l)
            for (unsigned i = 0; i < sizeof (percpu) / sizeof (*percpu); i++)
                if (percpu[i])
                    percpu[i]->update (mdb->node_base, o, phys, Ptab::hw_attr (a), rem);

        flush.merge (exist);
    }
}

void Space_mem::shootdown()
{
    for (unsigned cpu = 0; cpu < NUM_CPU; cpu++) {

        if (!Hip::cpu_online (cpu))
            continue;

        Pd *pd = Pd::remote (cpu);

        if (!pd->flush.chk (cpu))
            continue;

        if (Cpu::id == cpu) {
            Cpu::hazard |= Cpu::HZD_SCHED;
            continue;
        }

        unsigned ctr = Counter::remote (cpu, 1);

        Lapic::send_ipi (cpu, Lapic::DST_PHYSICAL, Lapic::DLV_FIXED, VEC_IPI_TLB);

        while (Counter::remote (cpu, 1) == ctr)
            pause();
    }
}

void Space_mem::insert_root (mword base, size_t size, mword attr)
{
    for (size_t frag; size; size -= frag) {

        unsigned type = Mtrr::memtype (base);

        for (frag = 0; frag < size; frag += PAGE_SIZE)
            if (Mtrr::memtype (base + frag) != type)
                break;

        size_t s = frag;

        for (unsigned o; s; s -= 1UL << o, base += 1UL << o)
            insert_node (new Mdb (&Pd::kern, base, o = max_order (base, s), attr, type));
    }
}

bool Space_mem::insert_utcb (mword b)
{
    if (!b)
        return true;

    Mdb *mdb = new Mdb (&Pd::kern, b, PAGE_BITS);

    if (insert_node (mdb))
        return true;

    delete mdb;

    return false;
}
