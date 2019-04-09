/*
 * Generic Timer
 *
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

#include "acpi.hpp"
#include "interrupt.hpp"
#include "stdio.hpp"
#include "timeout.hpp"
#include "timer.hpp"

void Timer::init()
{
    if (Cpu::bsp) {

        // Adjust system time
        if (Acpi::resume)
            offs = pct() - Acpi::resume;

        // Determine frequency of the system counter
        else
            asm volatile ("mrs %x0, cntfrq_el0" : "=r" (freq));
    }

    if (!Acpi::resume)
        trace (TRACE_TIMR, "TIMR: EL2p:%u%c EL1v:%u%c %lu Hz", ppi_el2_p, lvl_el2_p ? 'L' : 'E', ppi_el1_v, lvl_el1_v ? 'L' : 'E', freq);

    // Configure EL1v timer interrupt
    Interrupt::conf_ppi (ppi_el1_v, false, lvl_el1_v);

    // Configure EL2p timer interrupt
    Interrupt::conf_ppi (ppi_el2_p, false, lvl_el2_p);

    // Enable EL2p timer
    asm volatile ("msr cnthp_ctl_el2, %x0" : : "rZ" (BIT64 (0)));

    // Start timeout infrastructure
    Timeout::sync();
}
