/*
 * Memory Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

void Space_mem::init (unsigned cpu)
{
    if (cpus.set (cpu)) {
        loc[cpu].sync_from (Pd::kern.loc[cpu], CPU_LOCAL, SPC_LOCAL);
        loc[cpu].sync_master_range (LINK_ADDR, CPU_LOCAL);
    }
}

void Space_mem::update (mword v, mword p, unsigned o, Paging::Permissions pm, Memattr::Cacheability ca, Memattr::Shareability sh, Space::Index si)
{
    switch (si) {
        case Space::Index::CPU_HST: hpt.update (v, p, o, pm, ca, sh); break;
        case Space::Index::CPU_GST: ept.update (v, p, o, pm, ca, sh); break;
        case Space::Index::DMA_HST:
        case Space::Index::DMA_GST: dpt.update (v, p, o, pm, ca, sh); break;
    }
}

void Space_mem::flush (Space::Index si)
{
    switch (si) {
        case Space::Index::CPU_HST: hpt.flush(); break;
        case Space::Index::CPU_GST: ept.flush(); break;
        case Space::Index::DMA_HST:
        case Space::Index::DMA_GST: dpt.flush(); break;
    }
}

void Space_mem::shootdown()
{
    // FIXME: Enable preemption

    for (unsigned cpu = 0; cpu < NUM_CPU; cpu++) {

#if 0   // FIXME
        if (!Hip::hip->cpu_online (cpu))
            continue;
#endif

        Pd *pd = Pd::remote (cpu);

        if (!pd->htlb.chk (cpu) && !pd->gtlb.chk (cpu))
            continue;

        if (Cpu::id == cpu) {
            Cpu::hazard |= HZD_SCHED;
            continue;
        }

        auto cnt = Counter::req[Interrupt::Request::RKE].get (cpu);

        Interrupt::send_cpu (cpu, Interrupt::Request::RKE);

        while (Counter::req[Interrupt::Request::RKE].get (cpu) == cnt)
            pause();
    }
}
