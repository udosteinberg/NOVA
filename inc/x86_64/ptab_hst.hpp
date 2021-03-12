/*
 * Host Page Table (Stage-1)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

class Pte_hst final : public Pte_lpt<Pte_hst, uint64_t, uint64_t>
{
    friend class Pte;

    private:
        enum
        {
            ATTR_K      = BITN  (9),    // Kernel Memory
            ATTR_G      = BITN  (8),    // Global
        };

    public:
        // PTE attributes for PTAB at level l
        static constexpr auto attr_ptab (unsigned l)
        {
            assert (0 < l && l < lev());

            return ATTR_A | ATTR_U | ATTR_W | ATTR_P;
        }

        // PTE attributes for LEAF at level l
        static auto attr_leaf (unsigned l, Paging::Permissions p, Memattr a)
        {
            return !(p & Paging::API) ? 0 :
                     ATTR_D  * !!(p & (Paging::SS | Paging::W))         |
                     ATTR_G  * !!(p &  Paging::G)                       |
                     ATTR_K  * !!(p &  Paging::K)                       |
                     ATTR_U  * !!(p &  Paging::U)                       |
                     ATTR_nX *  !(p & (Paging::XS | Paging::XU))        |
                     ATTR_W  * !!(p &  Paging::W)                       |
                     ATTR_S  * !!l | ATTR_A | ATTR_P                    |
                     attr_ma (l, a);
        }

        constexpr auto page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                      !!(val & ATTR_G)  *  Paging::G                |
                                      !!(val & ATTR_K)  *  Paging::K                |
                                      !!(val & ATTR_U)  *  Paging::U                |
                                       !(val & ATTR_nX) * (Paging::XS | Paging::XU) |
                                      !!(val & ATTR_W)  *  Paging::W                |
                                      !!(val & ATTR_P)  *  Paging::R);
        }

        explicit constexpr Pte_hst() = default;
        explicit constexpr Pte_hst (Entry e) : Pte_lpt { e } {}
};

class Ptab_hst final : public Ptab<Pte_hst, uint64_t, uint64_t>
{
    friend class Space_hst;

    private:
        static Ptab_hst master;

    public:
        explicit constexpr Ptab_hst (OAddr v = 0) : Ptab { Pte_hst { v } } {}

        // Copy Constructor
        Ptab_hst (Ptab_hst const &x) : Ptab { Pte_hst { x.root.load() } } {}

        // Copy Assignment
        Ptab_hst& operator= (Ptab_hst const &x)
        {
            root.store (x.root.load());
            return *this;
        }

        ALWAYS_INLINE
        static Ptab_hst current()
        {
            uintptr_t val;
            asm volatile ("mov %%cr3, %0" : "=r" (val));
            return Ptab_hst { val & Pte_hst::addr_mask() };
        }

        ALWAYS_INLINE
        void make_current (uintptr_t pcid) const
        {
            asm volatile ("mov %0, %%cr3" : : "r" (root_addr() | pcid) : "memory");
        }

        ALWAYS_INLINE
        static void invalidate()
        {
            uintptr_t cr3;
            asm volatile ("mov %%cr3, %0; mov %0, %%cr3" : "=&r" (cr3) : : "memory");
        }

        ALWAYS_INLINE
        static void invalidate (uintptr_t addr)
        {
            asm volatile ("invlpg %0" : : "m" (*reinterpret_cast<uintptr_t *>(addr)) : "memory");
        }

        ALWAYS_INLINE
        static auto master_map (IAddr v, OAddr p, unsigned o, Paging::Permissions pm, Memattr ma)
        {
            return master.update (v, p, o, pm, ma);
        }

        bool share_from (Ptab_hst, IAddr, IAddr);
        void share_from_master (IAddr, IAddr);

        __attribute__((returns_nonnull)) static void *map (uintptr_t, OAddr, Paging::Permissions = Paging::R, Memattr = Memattr::ram(), unsigned = 2);

        [[nodiscard]] static void *map_tmp (OAddr, size_t, Paging::Permissions, Memattr, unsigned);
};

// Sanity Checks
static_assert (Pte_hst::lev() == 4);
static_assert (Pte_hst::lev_bit (0) == 9 && Pte_hst::lev_bit (1) == 9 && Pte_hst::lev_bit (2) == 9 && Pte_hst::lev_bit (3) == 9);
static_assert (Pte_hst::lev_idx (0, BITN (Pte_hst::ibits) - 1) == 511 && Pte_hst::lev_idx (1, BITN (Pte_hst::ibits) - 1) == 511 && Pte_hst::lev_idx (2, BITN (Pte_hst::ibits) - 1) == 511 && Pte_hst::lev_idx (3, BITN (Pte_hst::ibits) - 1) == 511);
