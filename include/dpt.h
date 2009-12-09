/*
 * DMA Page Table (DPT)
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

#include "buddy.h"
#include "compiler.h"
#include "types.h"
#include "x86.h"

class Dpt
{
    private:
        uint64 val;

        static unsigned const levels = 4;
        static unsigned const bpl = 9;

        ALWAYS_INLINE
        inline bool present() const { return val & (DPT_R | DPT_W); }

        ALWAYS_INLINE
        inline bool super() const { return val & DPT_SP; }

        ALWAYS_INLINE
        inline mword attr() const { return static_cast<mword>(val) & DPT_ATTR; }

        ALWAYS_INLINE
        inline Paddr addr() const { return static_cast<Paddr>(val) & DPT_ADDR; }

        ALWAYS_INLINE
        inline void set (uint64 v) { val = v; clflush (this); }

        Dpt *walk (uint64, unsigned long, bool);

    public:
        enum
        {
            DPT_0               = 0,
            DPT_R               = 1ul << 0,
            DPT_W               = 1ul << 1,
            DPT_SP              = 1ul << 7,
            DPT_SNP             = 1ul << 11,
            DPT_ATTR            =  ((1ul << 12) - 1),
            DPT_ADDR            = ~((1ul << 12) - 1)
        };

        void insert (uint64, mword, mword, uint64);
        void remove (uint64, mword);

        ALWAYS_INLINE
        inline Dpt *level (unsigned l) { return walk (0, l, true); }

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return Buddy::allocator.alloc (0, Buddy::FILL_0); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { Buddy::allocator.free (reinterpret_cast<mword>(ptr)); }
};
