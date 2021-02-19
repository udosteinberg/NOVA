/*
 * Host Page Table (HPT)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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

#include "ptab.hpp"

class Hpt final : public Pagetable<Hpt, uint64, uint64, 4, 3, false>::Entry
{
    private:
        enum
        {
            ATTR_P      = BIT64  (0),   // Present
            ATTR_W      = BIT64  (1),   // Writable
            ATTR_U      = BIT64  (2),   // User
            ATTR_A      = BIT64  (5),   // Accessed
            ATTR_D      = BIT64  (6),   // Dirty
            ATTR_S      = BIT64  (7),   // Superpage
            ATTR_G      = BIT64  (8),   // Global
            ATTR_K      = BIT64  (9),   // Kernel Memory
            ATTR_nX     = BIT64 (63),   // Not Executable
        };

    public:
        static constexpr OAddr ADDR_MASK { BIT64_RANGE (51, 12) };

        bool is_large (unsigned l) const { return l &&  (val & ATTR_S); }
        bool is_table (unsigned l) const { return l && !(val & ATTR_S); }

        // Attributes for PTEs referring to page tables
        static OAddr ptab_attr()
        {
            return ATTR_A | ATTR_U | ATTR_W | ATTR_P;
        }

        // Attributes for PTEs referring to leaf pages
        static OAddr page_attr (unsigned l, Paging::Permissions pm, Memattr::Cacheability ca, Memattr::Shareability)
        {
            return !(pm & Paging::API) ? 0 :
                     ATTR_D  * !!(pm & (Paging::SS | Paging::W))    |
                     ATTR_G  * !!(pm &  Paging::G)                  |
                     ATTR_K  * !!(pm &  Paging::K)                  |
                     ATTR_U  * !!(pm &  Paging::U)                  |
                     ATTR_nX *  !(pm & (Paging::XS | Paging::XU))   |
                     ATTR_W  * !!(pm &  Paging::W)                  |
                     ATTR_S  * !!l | ATTR_A | ATTR_P                |
                     (static_cast<OAddr>(ca) & BIT (2)) << (l ? 10 : 5) | (static_cast<OAddr>(ca) & BIT_RANGE (1, 0)) << 3;
        }

        Paging::Permissions page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                      !!(val & ATTR_G)  *  Paging::G                |
                                      !!(val & ATTR_K)  *  Paging::K                |
                                      !!(val & ATTR_U)  *  Paging::U                |
                                       !(val & ATTR_nX) * (Paging::XS | Paging::XU) |
                                      !!(val & ATTR_W)  *  Paging::W                |
                                      !!(val & ATTR_P)  *  Paging::R);
        }

        Memattr::Cacheability page_ca (unsigned l) const { return Memattr::Cacheability ((val >> (l ? 10 : 5) & BIT (2)) | (val >> 3 & BIT_RANGE (1, 0))); }

        Memattr::Shareability page_sh() const { return Memattr::Shareability::NONE; }

        // Needed by gcc version < 10
        ALWAYS_INLINE inline Hpt() : Entry() {}
        ALWAYS_INLINE inline Hpt (Entry x) : Entry (x) {}
};

class Hptp final : public Pagetable<Hpt, uint64, uint64, 4, 3, false>
{
    public:
        static Hptp master;

        // Constructor
        ALWAYS_INLINE
        inline explicit Hptp (OAddr v = 0) : Pagetable (Hpt (v)) {}

        // Copy Constructor
        ALWAYS_INLINE
        inline Hptp (Hptp const &x) : Pagetable (static_cast<Hpt>(x.root)) {}

        // Copy Assignment
        ALWAYS_INLINE
        inline Hptp& operator= (Hptp const &x)
        {
            root = static_cast<Hpt>(x.root);

            return *this;
        }

        ALWAYS_INLINE
        static inline Hptp current()
        {
            OAddr val;
            asm volatile ("mov %%cr3, %0" : "=r" (val));
            return Hptp (val & Hpt::ADDR_MASK);
        }

        ALWAYS_INLINE
        inline void make_current (uintptr_t pcid) const
        {
            asm volatile ("mov %0, %%cr3" : : "r" (root_addr() | pcid) : "memory");
        }

        ALWAYS_INLINE
        static inline void invalidate()
        {
            uintptr_t cr3;
            asm volatile ("mov %%cr3, %0; mov %0, %%cr3" : "=&r" (cr3));
        }

        ALWAYS_INLINE
        static inline void invalidate (uintptr_t addr)
        {
            asm volatile ("invlpg %0" : : "m" (*reinterpret_cast<uintptr_t *>(addr)));
        }

        bool sync_from (Hptp, IAddr, IAddr);

        void sync_from_master (IAddr, IAddr);

        static void *map (OAddr, bool = false);
};
