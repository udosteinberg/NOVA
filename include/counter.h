/*
 * Event Counters
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

#pragma once

#include "config.h"
#include "console_vga.h"
#include "cpu.h"

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
            return *reinterpret_cast<volatile unsigned *>(reinterpret_cast<mword>(ipi + i) - CPU_LOCAL_DATA + HV_GLOBAL_CPUS + c * PAGE_SIZE);
        }

        template <unsigned D, unsigned B>
        static void print (mword val, Console_vga::Color c, unsigned col)
        {
            if (EXPECT_FALSE (Cpu::row))
                for (unsigned i = 0; i < D; i++, val /= B)
                    Console_vga::con.put (Cpu::row, col - i, c, !i || val ? (val % B)["0123456789ABCDEF"] : ' ');
        }
};
