/*
 * Host Page Table (Stage-1)
 *
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

class Pte_hst final : public Pte<Pte_hst, uint64_t, uint64_t>
{
    friend class Pte;

    private:
        enum
        {
            ATTR_nX     = BITN (54),    // Not Executable
            ATTR_A      = BITN (10),    // Accessed
            ATTR_nW     = BITN  (7),    // Not Writable (AP[2])
            ATTR_RES1   = BITN  (6),    // RES1 (AP[1])
            ATTR_nS     = BITN  (5),    // Not Secure
            ATTR_nL     = BITN  (1),    // Not Large
            ATTR_P      = BITN  (0),    // Present
        };

        static_assert (!(DEAD_MARKER & ATTR_P));

    public:
        static constexpr unsigned ibits { 48 };

        // PTE attributes for PTAB at level l
        static constexpr auto attr_ptab (unsigned l)
        {
            assert (0 < l && l < lev());

            return ATTR_nL | ATTR_P;
        }

        // PTE attributes for LEAF at level l
        static constexpr auto attr_leaf (unsigned l, Paging::Permissions p, Memattr a)
        {
            assert (!(p & (Paging::K | Paging::XU)));

            return !(p & Paging::API) ? 0 :
                     ATTR_nX *  !(p & Paging::XS) |
                     ATTR_nW *  !(p & Paging::W)  |
                     a.share() << 8 | a.cache_s1() << 2 | !l * ATTR_nL | ATTR_RES1 | ATTR_A | ATTR_P;
        }

        constexpr auto page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                       !(val & ATTR_nX) * Paging::XS |
                                       !(val & ATTR_nW) * Paging::W  |
                                      !!(val & ATTR_P)  * Paging::R);
        }

        constexpr auto page_ma (unsigned) const
        {
            return Memattr { Memattr::Share (BIT_RANGE (1, 0) & val >> 8),
                             Memattr::Cache (BIT_RANGE (2, 0) & val >> 2) };
        }

        explicit constexpr Pte_hst() = default;
        explicit constexpr Pte_hst (Entry e) : Pte { e } {}
};

class Ptab_hst final : public Ptab<Pte_hst, uint64_t, uint64_t>
{
    private:
        static Ptab_hst master;

    public:
        explicit constexpr Ptab_hst (OAddr v = 0) : Ptab { Pte_hst { v } } {}

        ALWAYS_INLINE
        static Ptab_hst current()
        {
            uintptr_t val;
            asm volatile ("mrs %x0, ttbr0_el2" : "=r" (val));
            return Ptab_hst { val & Pte_hst::addr_mask() };
        }

        ALWAYS_INLINE
        static auto master_map (IAddr v, OAddr p, unsigned o, Paging::Permissions pm, Memattr ma)
        {
            return master.update (v, p, o, pm, ma);
        }

        ALWAYS_INLINE
        static void invalidate_self()
        {
            asm volatile ("tlbi alle2   ;"  // Invalidate TLB entries for SELF
                          "dsb  nsh     ;"  // Ensure TLB invalidation completed for SELF
                          "isb          ;"  // Ensure fetched instructions use new translation
                          : : : "memory");
        }

        bool share_from_master (IAddr);

        __attribute__((returns_nonnull)) static void *map (uintptr_t, OAddr, Paging::Permissions = Paging::R, Memattr = Memattr::ram(), unsigned = 2);

        [[nodiscard]] static void *map_tmp (OAddr, size_t, Paging::Permissions, Memattr, unsigned);
};

// Sanity Checks
static_assert (Pte_hst::lev() == 4);
static_assert (Pte_hst::lev_bit (0) == 9 && Pte_hst::lev_bit (1) == 9 && Pte_hst::lev_bit (2) == 9 && Pte_hst::lev_bit (3) == 9);
static_assert (Pte_hst::lev_idx (0, BITN (Pte_hst::ibits) - 1) == 511 && Pte_hst::lev_idx (1, BITN (Pte_hst::ibits) - 1) == 511 && Pte_hst::lev_idx (2, BITN (Pte_hst::ibits) - 1) == 511 && Pte_hst::lev_idx (3, BITN (Pte_hst::ibits) - 1) == 511);
