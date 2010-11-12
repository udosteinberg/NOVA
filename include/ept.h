/*
 * Extended Page Table (EPT)
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

#pragma once

#include "assert.h"
#include "compiler.h"
#include "pte.h"

class Ept : public Pte<Ept, uint64, 4, 9>
{
    public:
        static mword ord;

        enum
        {
            EPT_R   = 1UL << 0,
            EPT_W   = 1UL << 1,
            EPT_X   = 1UL << 2,
            EPT_I   = 1UL << 6,
            EPT_S   = 1UL << 7,

            PTE_P   = EPT_R | EPT_W | EPT_X,
            PTE_S   = EPT_S,
            PTE_N   = EPT_R | EPT_W | EPT_X,
        };

        ALWAYS_INLINE
        static inline mword hw_attr (mword a, mword t) { return a ? t << 3 | a | EPT_I | EPT_R : 0; }

        ALWAYS_INLINE
        inline void flush()
        {
            uint64 eptp = addr() | (max() - 1) << 3 | 6;

            bool ret;
            asm volatile ("invept %1, %2; seta %0" : "=q" (ret) : "m" (eptp), "r" (1UL) : "cc");
            assert (ret);
        }
};
