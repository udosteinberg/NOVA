/*
 * Page Table Entry (PTE)
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

#include "buddy.h"
#include "compiler.h"

template <typename P, typename E, unsigned L, unsigned B>
class Pte
{
    protected:
        E val;

        ALWAYS_INLINE
        inline void set (E v) { val = v; }

        ALWAYS_INLINE
        inline bool present() const { return val & P::PTE_P; }

        ALWAYS_INLINE
        inline bool super() const { return val & P::PTE_S; }

        ALWAYS_INLINE
        inline mword attr() const { return static_cast<mword>(val) & PAGE_MASK; }

        ALWAYS_INLINE
        inline Paddr addr() const { return static_cast<Paddr>(val) & ~PAGE_MASK; }

        P *walk (E, unsigned long, bool = false);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return Buddy::allocator.alloc (0, Buddy::FILL_0); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { Buddy::allocator.free (reinterpret_cast<mword>(ptr)); }

    public:
        ALWAYS_INLINE
        static inline unsigned bpl() { return B; }

        ALWAYS_INLINE
        static inline unsigned max() { return L; }

        ALWAYS_INLINE
        inline E root (mword l = L - 1) { return Buddy::ptr_to_phys (walk (0, l)); }

        size_t lookup (E, Paddr &);

        bool update (E, mword, E, mword, bool = false);
};
