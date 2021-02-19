/*
 * Page Table Entry (PTE)
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

#include "buddy.hpp"
#include "cache.hpp"
#include "kmem.hpp"
#include "lowlevel.hpp"

template <typename P, typename E, unsigned L, unsigned B, bool F>
class Pte
{
    protected:
        E val;

        P *walk (E, unsigned long, bool = true);

        ALWAYS_INLINE
        inline bool present() const { return val & P::PTE_P; }

        ALWAYS_INLINE
        inline bool super() const { return val & P::PTE_S; }

        ALWAYS_INLINE
        inline mword attr() const { return static_cast<mword>(val) & OFFS_MASK; }

        ALWAYS_INLINE
        inline Paddr addr() const { return static_cast<Paddr>(val) & ~((1UL << order()) - 1); }

        ALWAYS_INLINE
        inline mword order() const { return PAGE_BITS; }

        ALWAYS_INLINE
        static inline mword order (mword) { return 0; }

        ALWAYS_INLINE
        inline bool set (E o, E v)
        {
            bool b = __atomic_compare_exchange_n (&val, &o, v, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);

            if (F && b)
                Cache::data_clean (this);

            return b;
        }

        ALWAYS_INLINE
        static inline void *operator new (size_t)
        {
            void *p = Buddy::allocator.alloc (0, Buddy::FILL_0);

            if (F)
                Cache::data_clean (p, PAGE_SIZE);

            return p;
        }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { Buddy::allocator.free (reinterpret_cast<mword>(ptr)); }

    public:
        enum
        {
            ERR_P   = 1UL << 0,
            ERR_W   = 1UL << 1,
            ERR_U   = 1UL << 2,
        };

        enum Type
        {
            TYPE_UP,
            TYPE_DN,
            TYPE_DF,
        };

        ALWAYS_INLINE
        static inline unsigned bpl() { return B; }

        ALWAYS_INLINE
        static inline unsigned max() { return L; }

        ALWAYS_INLINE
        inline E root (mword l = L - 1) { return Kmem::ptr_to_phys (walk (0, l)); }

        size_t lookup (E, Paddr &, mword &);

        void update (E, mword, E, mword, Type = TYPE_UP);
};
