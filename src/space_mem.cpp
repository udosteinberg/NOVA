/*
 * Memory Space
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
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

#include "bits.h"
#include "counter.h"
#include "hazards.h"
#include "hip.h"
#include "lapic.h"
#include "mtrr.h"
#include "pd.h"

unsigned Space_mem::did_ctr;

void Space_mem::init (unsigned cpu)
{
    if (cpus.set (cpu)) {

        loc[cpu].sync_from (Pd::kern.loc[cpu], LOCAL_SADDR);

        // Sync kernel code and data
        loc[cpu].sync_master_range (LINK_ADDR, LOCAL_SADDR);

        trace (TRACE_MEMORY, "PD:%p PTAB[%u]:%#lx", static_cast<Pd *>(this), cpu, loc[cpu].addr());
    }
}

void Space_mem::update (Mdb *mdb, Paddr phys, mword r, mword s)
{
    assert (this == mdb->node_pd && this != &Pd::kern);

    Lock_guard <Spinlock> guard (mdb->node_lock);

    mword b = mdb->node_base << PAGE_BITS;
    mword o = mdb->node_order;
    mword a = mdb->node_attr & ~r;

    if (s & 1 && Dpt::ord != ~0UL) {
        mword ord = min (o, Dpt::ord);
        for (unsigned long i = 0; i < 1UL << (o - ord); i++)
            dpt.update (b + i * (1UL << (Dpt::ord + PAGE_BITS)), ord, phys + i * (1UL << (Dpt::ord + PAGE_BITS)), a, r);
    }

    if (s & 2 && Ept::ord != ~0UL) {
        mword ord = min (o, Ept::ord);
        for (unsigned long i = 0; i < 1UL << (o - ord); i++)
            ept.update (b + i * (1UL << (Ept::ord + PAGE_BITS)), ord, phys + i * (1UL << (Ept::ord + PAGE_BITS)), Ept::hw_attr (a), mdb->node_type, r);
        if (r)
            gtlb.merge (cpus);
    }

    bool l = hpt.update (b, o, phys, Hpt::hw_attr (a), r);

    if (r) {

        if (l)
            for (unsigned i = 0; i < sizeof (loc) / sizeof (*loc); i++)
                if (loc[i].addr())
                    loc[i].update (b, o, phys, Hpt::hw_attr (a), r);

        htlb.merge (cpus);
    }
}

void Space_mem::shootdown()
{
    for (unsigned cpu = 0; cpu < NUM_CPU; cpu++) {

        if (!Hip::cpu_online (cpu))
            continue;

        Pd *pd = Pd::remote (cpu);

        if (!pd->htlb.chk (cpu))
            continue;

        if (Cpu::id == cpu) {
            Cpu::hazard |= HZD_SCHED;
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

        for (unsigned long o; s; s -= 1UL << o, base += 1UL << o)
            insert_node (new Mdb (&Pd::kern, base >> PAGE_BITS, (o = max_order (base, s)) - PAGE_BITS, attr, type));
    }
}

bool Space_mem::insert_utcb (mword b)
{
    if (!b)
        return true;

    Mdb *mdb = new Mdb (&Pd::kern, b >> PAGE_BITS, 0);

    if (insert_node (mdb))
        return true;

    delete mdb;

    return false;
}
