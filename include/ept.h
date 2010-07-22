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
#include "buddy.h"
#include "compiler.h"

class Ept
{
    private:
        uint64 val;

        enum
        {
            EPT_R   = 1UL << 0,
            EPT_W   = 1UL << 1,
            EPT_X   = 1UL << 2,
            EPT_I   = 1UL << 6,
            EPT_S   = 1UL << 7,
        };

        ALWAYS_INLINE
        inline bool present() const { return val & (EPT_R | EPT_W | EPT_X); }

        ALWAYS_INLINE
        inline bool super() const { return val & EPT_S; }

        ALWAYS_INLINE
        inline mword attr() const { return static_cast<mword>(val) & 0xfff; }

        ALWAYS_INLINE
        inline void set (uint64 v) { val = v; }

        Ept *walk (uint64, unsigned long, bool = false);

    public:
        static unsigned const bpl = 9;
        static unsigned const max = 4;
        static mword ord;

        ALWAYS_INLINE
        static inline mword hw_attr (mword a) { return a ? a | EPT_R : 0; }

        ALWAYS_INLINE
        inline Paddr addr() const { return static_cast<Paddr>(val) & ~0xfff; }

        void update (uint64, mword, uint64, mword, mword, bool = false);

        ALWAYS_INLINE
        inline void flush()
        {
            bool ret;
            asm volatile ("invept %1, %2; seta %0" : "=q" (ret) : "m" (val), "r" (1UL) : "cc");
            assert (ret);
        }

        ALWAYS_INLINE
        inline uint64 root() { walk (0, max - 1); return val; }

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return Buddy::allocator.alloc (0, Buddy::FILL_0); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { Buddy::allocator.free (reinterpret_cast<mword>(ptr)); }
};
