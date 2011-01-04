/*
 * Event Counters
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

#pragma once

#include "compiler.h"
#include "stdio.h"

class Counter
{
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

        static void dump();

        ALWAYS_INLINE
        static inline unsigned remote (unsigned c, unsigned i)
        {
            return *reinterpret_cast<volatile unsigned *>(reinterpret_cast<mword>(ipi + i) - CPULC_ADDR + CPUGL_ADDR + c * PAGE_SIZE);
        }

        static void print (unsigned val, Console_vga::Color c, unsigned col)
        {
            if (EXPECT_FALSE (Cpu::row))
                screen.put (Cpu::row, col, c, (val & 0xf)["0123456789ABCDEF"]);
        }
};
