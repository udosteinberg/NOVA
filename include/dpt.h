/*
 * DMA Page Table (DPT)
 *
 * Copyright (C) 2009-2010, Udo Steinberg <udo@hypervisor.org>
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
#include "x86.h"

class Dpt
{
    private:
        uint64 val;

        ALWAYS_INLINE
        inline bool present() const { return val & (DPT_R | DPT_W); }

        ALWAYS_INLINE
        inline bool super() const { return val & DPT_S; }

        ALWAYS_INLINE
        inline mword attr() const { return static_cast<mword>(val) & 0xfff; }

        ALWAYS_INLINE
        inline Paddr addr() const { return static_cast<Paddr>(val) & ~0xfff; }

        ALWAYS_INLINE
        inline void set (uint64 v) { val = v; clflush (this); }

        Dpt *walk (uint64, unsigned long, mword = 0);

    public:
        static unsigned const bpl = 9;
        static unsigned const max = 4;
        static unsigned ord;

        enum
        {
            DPT_R               = 1ul << 0,
            DPT_W               = 1ul << 1,
            DPT_S               = 1ul << 7,
        };

        ALWAYS_INLINE
        inline Dpt()
        {
            for (unsigned i = 0; i < PAGE_SIZE / sizeof (*this); i++)
                clflush (this + i);
        }

        void insert (uint64, mword, uint64, mword);
        void remove (uint64, mword);

        ALWAYS_INLINE
        inline Dpt *level (unsigned l) { return walk (0, l, DPT_R | DPT_W); }

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return Buddy::allocator.alloc (0, Buddy::FILL_0); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { Buddy::allocator.free (reinterpret_cast<mword>(ptr)); }
};
