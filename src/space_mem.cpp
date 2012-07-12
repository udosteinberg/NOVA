/*
 * Memory Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#include "counter.h"
#include "hazards.h"
#include "hip.h"
#include "lapic.h"
#include "mtrr.h"
#include "pd.h"
#include "stdio.h"
#include "svm.h"
#include "vectors.h"

unsigned Space_mem::did_ctr;

void Space_mem::init (unsigned cpu)
{
    if (cpus.set (cpu)) {
        loc[cpu].sync_from (Pd::kern.loc[cpu], CPU_LOCAL, SPC_LOCAL);
        loc[cpu].sync_master_range (LINK_ADDR, CPU_LOCAL);
    }
}

void Space_mem::update (Mdb *mdb, mword r)
{
    assert (this == mdb->space && this != &Pd::kern);

    Lock_guard <Spinlock> guard (mdb->node_lock);

    Paddr p = mdb->node_phys << PAGE_BITS;
    mword b = mdb->node_base << PAGE_BITS;
    mword o = mdb->node_order;
    mword a = mdb->node_attr & ~r;
    mword s = mdb->node_sub;

    if (s & 1 && Dpt::ord != ~0UL) {
        mword ord = min (o, Dpt::ord);
        for (unsigned long i = 0; i < 1UL << (o - ord); i++)
            dpt.update (b + i * (1UL << (ord + PAGE_BITS)), ord, p + i * (1UL << (Dpt::ord + PAGE_BITS)), a, r);
    }

    if (s & 2) {
        if (Vmcb::has_npt()) {
            mword ord = min (o, Hpt::ord);
            for (unsigned long i = 0; i < 1UL << (o - ord); i++)
                npt.update (b + i * (1UL << (ord + PAGE_BITS)), ord, p + i * (1UL << (ord + PAGE_BITS)), Hpt::hw_attr (a), r);
        } else {
            mword ord = min (o, Ept::ord);
            for (unsigned long i = 0; i < 1UL << (o - ord); i++)
                ept.update (b + i * (1UL << (ord + PAGE_BITS)), ord, p + i * (1UL << (ord + PAGE_BITS)), Ept::hw_attr (a, mdb->node_type), r);
        }
        if (r)
            gtlb.merge (cpus);
    }

    if (mdb->node_base + (1UL << o) > USER_ADDR >> PAGE_BITS)
        return;

    bool l = hpt.update (b, o, p, Hpt::hw_attr (a), r);

    if (r) {

        if (l)
            for (unsigned i = 0; i < sizeof (loc) / sizeof (*loc); i++)
                if (loc[i].addr())
                    loc[i].update (b, o, p, Hpt::hw_attr (a), r);

        htlb.merge (cpus);
    }
}

void Space_mem::shootdown()
{
    for (unsigned cpu = 0; cpu < NUM_CPU; cpu++) {

        if (!Hip::cpu_online (cpu))
            continue;

        Pd *pd = Pd::remote (cpu);

        if (!pd->htlb.chk (cpu) && !pd->gtlb.chk (cpu))
            continue;

        if (Cpu::id == cpu) {
            Cpu::hazard |= HZD_SCHED;
            continue;
        }

        unsigned ctr = Counter::remote (cpu, 1);

        Lapic::send_ipi (cpu, VEC_IPI_RKE);

        while (Counter::remote (cpu, 1) == ctr)
            pause();
    }
}

void Space_mem::insert_root (uint64 s, uint64 e, mword a)
{
    for (uint64 p = s; p < e; s = p) {

        unsigned t = Mtrr::memtype (s, p);

        for (uint64 n; p < e; p = n)
            if (Mtrr::memtype (p, n) != t)
                break;

        if (s > ~0UL)
            break;

        if ((p = min (p, e)) > ~0UL)
            p = static_cast<uint64>(~0UL) + 1;

        addreg (static_cast<mword>(s >> PAGE_BITS), static_cast<mword>(p - s) >> PAGE_BITS, a, t);
    }
}

bool Space_mem::insert_utcb (mword b)
{
    if (!b)
        return true;

    Mdb *mdb = new Mdb (this, 0, b >> PAGE_BITS, 0);

    if (tree_insert (mdb))
        return true;

    delete mdb;

    return false;
}
