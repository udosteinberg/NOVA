/*
 * Host Page Table (HPT)
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
#include "user.h"

class Hpt
{
    friend class Vtlb;

    private:
        mword val;

        ALWAYS_INLINE
        inline bool present() const { return val & HPT_P; }

        ALWAYS_INLINE
        inline bool super() const { return val & HPT_S; }

        ALWAYS_INLINE
        inline bool accessed() const { return val & HPT_A; }

        ALWAYS_INLINE
        inline bool dirty() const { return val & HPT_D; }

        ALWAYS_INLINE
        inline mword attr() const { return static_cast<mword>(val) & 0xfff; }

        ALWAYS_INLINE
        static inline void flush()
        {
            mword cr3;
            asm volatile ("mov %%cr3, %0; mov %0, %%cr3" : "=&r" (cr3));
        }

        Hpt *walk (mword, unsigned long, bool = false);

    public:
        static unsigned const bpl = 10;
        static unsigned const max = 2;

        enum
        {
            HPT_P   = 1UL << 0,
            HPT_W   = 1UL << 1,
            HPT_U   = 1UL << 2,
            HPT_UC  = 1UL << 4,
            HPT_A   = 1UL << 5,
            HPT_D   = 1UL << 6,
            HPT_S   = 1UL << 7,
            HPT_G   = 1UL << 8,
            HPT_NX  = 0,
        };

        ALWAYS_INLINE
        inline explicit Hpt (mword v = 0) : val (v) {}

        ALWAYS_INLINE
        inline void set (Paddr v) { val = v; }

        ALWAYS_INLINE
        inline Paddr addr() const { return static_cast<Paddr>(val) & ~0xfff; }

        ALWAYS_INLINE
        static inline mword hw_attr (mword a) { return a ? a | HPT_U | HPT_P : 0; }

        ALWAYS_INLINE
        static inline mword current()
        {
            mword addr;
            asm volatile ("mov %%cr3, %0" : "=r" (addr));
            return addr;
        }

        ALWAYS_INLINE
        inline void make_current()
        {
            asm volatile ("mov %0, %%cr3" : : "r" (val) : "memory");
        }

        ALWAYS_INLINE
        inline bool mark (Hpt *e, mword bits)
        {
            return EXPECT_TRUE ((e->val & bits) == bits) || User::cmp_swap (&val, e->val, e->val | bits) == ~0UL;
        }

        size_t lookup (mword, Paddr &);

        bool update (mword, mword, Paddr, mword, bool = false);

        bool sync_from (Hpt, mword);

        size_t sync_master (mword virt);
        void sync_master_range (mword s_addr, mword e_addr);

        static void *remap (Paddr phys);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return Buddy::allocator.alloc (0, Buddy::FILL_0); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { Buddy::allocator.free (reinterpret_cast<mword>(ptr)); }
};
