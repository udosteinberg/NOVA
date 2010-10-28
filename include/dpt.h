/*
 * DMA Page Table (DPT)
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

#include "compiler.h"
#include "pte.h"
#include "x86.h"

class Dpt : public Pte<Dpt, uint64, 4, 9>
{
    friend class Pte<Dpt, uint64, 4, 9>;

    private:
        ALWAYS_INLINE
        inline void set (uint64 v) { val = v; flush (this); }

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return flush (Buddy::allocator.alloc (0, Buddy::FILL_0), PAGE_SIZE); }

    public:
        static mword ord;

        enum
        {
            DPT_R   = 1UL << 0,
            DPT_W   = 1UL << 1,
            DPT_S   = 1UL << 7,

            PTE_P   = DPT_R | DPT_W,
            PTE_S   = DPT_S,
            PTE_N   = DPT_R | DPT_W,
        };
};
