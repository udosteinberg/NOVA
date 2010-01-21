/*
 * Extended Page Table (EPT)
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
#include "types.h"

class Ept;

class Eptp
{
    private:
        uint64 val;

    public:
        ALWAYS_INLINE
        inline Eptp (Ept *ept) : val (Buddy::ptr_to_phys (ept) | 0x1e) {}
};

class Invept : public Eptp
{
    private:
        uint64 res;

    public:
        ALWAYS_INLINE
        inline Invept (Ept *ept) : Eptp (ept) {}
};

class Ept
{
    private:
        uint64 val;

        ALWAYS_INLINE
        inline bool present() const { return val & (EPT_R | EPT_W | EPT_X); }

        ALWAYS_INLINE
        inline bool super() const { return val & EPT_S; }

        ALWAYS_INLINE
        inline mword attr() const { return static_cast<mword>(val) & 0xfff; }

        ALWAYS_INLINE
        inline Paddr addr() const { return static_cast<Paddr>(val) & ~0xfff; }

        ALWAYS_INLINE
        inline void set (uint64 v) { val = v; }

        ALWAYS_INLINE
        inline void flush() { asm volatile ("invept %0, %1" : : "m" (Invept (this)), "r" (1ul) : "cc"); }

        Ept *walk (uint64, unsigned long, mword = 0);

    public:
        static unsigned const bpl = 9;
        static unsigned const max = 4;
        static unsigned ord;

        enum
        {
            EPT_R               = 1ul << 0,
            EPT_W               = 1ul << 1,
            EPT_X               = 1ul << 2,
            EPT_I               = 1ul << 6,
            EPT_S               = 1ul << 7,
        };

        void insert (uint64, mword, uint64, mword, mword);
        void remove (uint64, mword);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return Buddy::allocator.alloc (0, Buddy::FILL_0); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { Buddy::allocator.free (reinterpret_cast<mword>(ptr)); }
};
