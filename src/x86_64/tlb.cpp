/*
 * Translation Lookaside Buffer (TLB)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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
#include "ec.hpp"
#include "interrupt.hpp"
#include "space_gst.hpp"
#include "space_hst.hpp"
#include "stc.hpp"
#include "stdio.hpp"
#include "tlb.hpp"

void Tlb::shootdown (Space *s)
{
    auto const t { Stc::ms_to_ticks (1) };

    Cpu::preemption_enable();

    for (cpu_t cpu { 0 }; cpu < Cpu::count; cpu++) {

        auto const ec { Ec::remote_current (cpu) };

        if (ec->get_hst() != s && ec->get_gst() != s)
            continue;

        if (Cpu::id == cpu) {
            Cpu::hazard |= Hazard::SCHED;
            continue;
        }

        auto const c { Counter::req[Interrupt::Request::RKE].get (cpu) };

        Interrupt::send_cpu (Interrupt::Request::RKE, cpu);

        for (auto const b { __builtin_ia32_rdtsc() }; Counter::req[Interrupt::Request::RKE].get (cpu) == c; __builtin_ia32_pause())
            if (EXPECT_FALSE (__builtin_ia32_rdtsc() - b > t)) {
                trace (TRACE_ERROR, "CPU:%u %s hang ", cpu, __func__);
                break;
            }
    }

    Cpu::preemption_disable();
}
