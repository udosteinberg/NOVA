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

#include "counter.hpp"
#include "hazards.hpp"
#include "hip.hpp"
#include "interrupt.hpp"
#include "lowlevel.hpp"
#include "mtrr.hpp"
#include "pd.hpp"
#include "stdio.hpp"
#include "svm.hpp"
#include "vectors.hpp"

unsigned Space_mem::did_ctr;

void Space_mem::init (unsigned cpu)
{
    if (!cpus.tas (cpu)) {
        loc[cpu].share_from (Pd::kern.loc[cpu], MMAP_CPU, MMAP_SPC);
        loc[cpu].share_from_master (LINK_ADDR, MMAP_CPU);
    }
}

void Space_mem::shootdown()
{
    for (unsigned cpu = 0; cpu < NUM_CPU; cpu++) {

        if (!Hip::hip->cpu_online (cpu))
            continue;

        Pd *pd = Pd::remote (cpu);

        if (!pd->htlb.tst (cpu) && !pd->gtlb.tst (cpu))
            continue;

        if (Cpu::id == cpu) {
            Cpu::hazard |= HZD_SCHED;
            continue;
        }

        auto ctr = Counter::req[1].get (cpu);

        Interrupt::send_cpu (Interrupt::Request::RKE, cpu);

        while (Counter::req[1].get (cpu) == ctr)
            pause();
    }
}

void Space_mem::insert_root (uint64 s, uint64 e, mword)
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

#if 0   // FIXME
        addreg (static_cast<mword>(s >> PAGE_BITS), static_cast<mword>(p - s) >> PAGE_BITS, a, t);
#endif
    }
}

bool Space_mem::insert_utcb (mword b)
{
    if (!b)
        return true;

    return false;
}
