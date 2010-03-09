/*
 * Event Counters
 *
 * Copyright (C) 2009, Udo Steinberg <udo@hypervisor.org>
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

#pragma once

#include "compiler.h"
#include "stdio.h"

class Counter
{
    private:
        static unsigned row             CPULOCAL;

    public:
        static unsigned ipi[NUM_IPI]    CPULOCAL;
        static unsigned lvt[NUM_LVT]    CPULOCAL;
        static unsigned gsi[NUM_GSI]    CPULOCAL;
        static unsigned exc[NUM_EXC]    CPULOCAL;
        static unsigned vmi[NUM_VMI]    CPULOCAL;
        static unsigned vtlb_gpf        CPULOCAL;
        static unsigned vtlb_hpf        CPULOCAL;
        static unsigned vtlb_fill       CPULOCAL;
        static unsigned vtlb_flush      CPULOCAL;
        static unsigned schedule        CPULOCAL;
        static unsigned helping         CPULOCAL;
        static uint64   cycles_idle     CPULOCAL;

        static void init();
        static void dump();

        static void count (unsigned &counter, Console_vga::Color c, unsigned col)
        {
            counter++;

            if (EXPECT_FALSE (row))
                screen.put (row, col, c, (counter & 0xf)["0123456789ABCDEF"]);
        }
};
