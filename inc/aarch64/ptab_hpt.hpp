/*
 * Host Page Table (HPT)
 *
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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
            ATTR_nL     = BIT64  (1),   // Not Large
            ATTR_nS     = BIT64  (5),   // Not Secure
            ATTR_U      = BIT64  (6),   // User
            ATTR_nW     = BIT64  (7),   // Not Writable
            ATTR_A      = BIT64 (10),   // Accessed
            ATTR_nG     = BIT64 (11),   // Not Global
            ATTR_nX     = BIT64 (54),   // Not Executable
            ATTR_K      = BIT64 (55),   // Kernel Memory
        };

    public:
        static constexpr OAddr ADDR_MASK = BIT64_RANGE (47, 12);

        bool is_large (unsigned l) const { return              l && (val & (ATTR_nL | ATTR_P)) == (ATTR_P); }
        bool is_table (unsigned l) const { return l == lev || (l && (val & (ATTR_nL | ATTR_P)) == (ATTR_nL | ATTR_P)); }

        // Attributes for PTEs referring to page tables
        static OAddr ptab_attr()
        {
            return ATTR_nL | ATTR_P;
        }

        // Attributes for PTEs referring to leaf pages
        static OAddr page_attr (unsigned l, Paging::Permissions pm, Memattr::Cacheability ca, Memattr::Shareability sh)
        {
            return !(pm & Paging::API) ? 0 :
                     ATTR_nG *  !(pm & Paging::G)  |
                     ATTR_K  * !!(pm & Paging::K)  |
                     ATTR_U  * !!(pm & Paging::U)  |
                     ATTR_nX *  !(pm & Paging::XS) |
                     ATTR_nW *  !(pm & Paging::W)  |
                     (static_cast<OAddr>(sh) & 0x3) << 8 | (static_cast<OAddr>(ca) & 0x7) << 2 | !l * ATTR_nL | ATTR_A | ATTR_P;
        }

        Paging::Permissions page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                       !(val & ATTR_nG) * Paging::G  |
                                      !!(val & ATTR_K)  * Paging::K  |
                                      !!(val & ATTR_U)  * Paging::U  |
                                       !(val & ATTR_nX) * Paging::XS |
                                       !(val & ATTR_nW) * Paging::W  |
                                      !!(val & ATTR_P)  * Paging::R);
        }

        Memattr::Cacheability page_ca (unsigned) const { return Memattr::Cacheability (val >> 2 & 0x7); }

        Memattr::Shareability page_sh() const { return Memattr::Shareability (val >> 8 & 0x3); }

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

        ALWAYS_INLINE
        static inline Hptp current()
        {
            OAddr val;
            asm volatile ("mrs %0, ttbr0_el2" : "=r" (val));
            return Hptp (val & Hpt::ADDR_MASK);
        }

        ALWAYS_INLINE
        inline void make_current() const
        {
            asm volatile ("msr ttbr0_el2, %0; isb" : : "r" (root_addr()) : "memory");
        }

        ALWAYS_INLINE
        static inline void invalidate_cpu()
        {
            asm volatile ("dsb  nshst   ;"  // Ensure PTE writes have completed
                          "tlbi alle2   ;"  // Invalidate TLB
                          "dsb  nsh     ;"  // Ensure TLB invalidation completed
                          "isb          ;"  // Ensure subsequent instructions use new translation
                          : : : "memory");
        }

        bool sync_from_master (IAddr);

        static void *map (OAddr, bool = false);
};
