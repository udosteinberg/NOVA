/*
 * Host Page Table (HPT)
 *
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

#include "ptab_pte.hpp"

class Hpt final : public Pte<Hpt, uint64, uint64>
{
    friend class Pte;

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
        };

    public:
        static constexpr unsigned obits { 48 };
        static constexpr unsigned ibits { 48 };
        static constexpr auto ptab_attr { ATTR_nL | ATTR_P };

        // Attributes for PTEs referring to leaf pages
        static OAddr page_attr (unsigned l, Paging::Permissions pm, Memattr::Cacheability ca, Memattr::Shareability sh)
        {
            assert (!(pm & (Paging::K | Paging::XU)));

            return !(pm & Paging::API) ? 0 :
                     ATTR_nG *  !(pm & Paging::G)  |
                     ATTR_U  * !!(pm & Paging::U)  |
                     ATTR_nX *  !(pm & Paging::XS) |
                     ATTR_nW *  !(pm & Paging::W)  |
                     std::to_underlying (sh) << 8 | std::to_underlying (ca) << 2 | !l * ATTR_nL | ATTR_A | ATTR_P;
        }

        Paging::Permissions page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                       !(val & ATTR_nG) * Paging::G  |
                                      !!(val & ATTR_U)  * Paging::U  |
                                       !(val & ATTR_nX) * Paging::XS |
                                       !(val & ATTR_nW) * Paging::W  |
                                      !!(val & ATTR_P)  * Paging::R);
        }

        Memattr::Cacheability page_ca (unsigned) const { return Memattr::Cacheability (val >> 2 & BIT_RANGE (2, 0)); }

        Memattr::Shareability page_sh() const { return Memattr::Shareability (val >> 8 & BIT_RANGE (1, 0)); }

        Hpt() = default;
        Hpt (Entry e) : Pte (e) {}
};

class Hptp final : public Ptab<Hpt, uint64, uint64>
{
    private:
        static Hptp master;

    public:
        explicit Hptp (OAddr v = 0) : Ptab (Hpt (v)) {}

        ALWAYS_INLINE
        static inline Hptp current()
        {
            OAddr val;
            asm volatile ("mrs %x0, ttbr0_el2" : "=r" (val));
            return Hptp (val & Hpt::addr_mask());
        }

        ALWAYS_INLINE
        inline void make_current() const
        {
            asm volatile ("msr ttbr0_el2, %x0; isb" : : "rZ" (root_addr()) : "memory");
        }

        ALWAYS_INLINE
        static inline void master_map (IAddr v, OAddr p, unsigned o, Paging::Permissions pm, Memattr::Cacheability ca, Memattr::Shareability sh)
        {
            master.update (v, p, o, pm, ca, sh);
        }

        ALWAYS_INLINE
        static inline void invalidate_cpu()
        {
            asm volatile ("tlbi alle2   ;"  // Invalidate TLB entries
                          "dsb  nsh     ;"  // Ensure TLB invalidation completed
                          "isb          ;"  // Ensure fetched instructions use new translation
                          : : : "memory");
        }

        bool share_from_master (IAddr);

        static void *map (OAddr, bool = false);
};

// Sanity checks
static_assert (Hpt::lev() == 4);
static_assert (Hpt::lev_bit (0) == 9 && Hpt::lev_bit (1) == 9 && Hpt::lev_bit (2) == 9 && Hpt::lev_bit (3) == 9);
static_assert (Hpt::lev_idx (0, BITN (Hpt::ibits) - 1) == 511 && Hpt::lev_idx (1, BITN (Hpt::ibits) - 1) == 511 && Hpt::lev_idx (2, BITN (Hpt::ibits) - 1) == 511 && Hpt::lev_idx (3, BITN (Hpt::ibits) - 1) == 511);
