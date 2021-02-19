/*
 * DMA Page Table (AMD IOMMU Format)
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

class Pte_dma_amd final : public Pte<Pte_dma_amd, uint64_t, uint64_t>
{
    friend class Pte;

    private:
        enum
        {
            ATTR_W      = BITN (62),    // Writable
            ATTR_R      = BITN (61),    // Readable
            ATTR_FC     = BITN (60),    // Force Coherent
            ATTR_U      = BITN (59),    // Untranslated
            ATTR_D      = BITN  (6),    // Dirty
            ATTR_A      = BITN  (5),    // Accessed
            ATTR_P      = BITN  (0),    // Present
        };

        static_assert (!(DEAD_MARKER & ATTR_P));

    public:
        static constexpr unsigned ibits { 57 };

        // TLB invalidation requirements for AMD IOMMU
        static        constexpr bool inv_upgrade    { true  };  // Any PTE change requires invalidation
        static        constexpr bool inv_splinter   { true  };  // Any PTE change requires invalidation
        static inline constinit bool inv_notpresent { false };  // True if SMMU caches not-present entries

        // PTE type at level l (root[11:0] is always zero)
        constexpr auto type (unsigned l) const
        {
            if (val & ATTR_P)
                return val & BIT_RANGE (11, 9) ? Type::PTAB : Type::LEAF;

            // The root slot carries no attributes and is never DEAD: nonzero means PTAB
            return val ? (l == lev() ? Type::PTAB : Type::DEAD) : Type::HOLE;
        }

        // PTE attributes for PTAB at level l with [11:9] == l
        static constexpr auto attr_ptab (unsigned l)
        {
            assert (0 < l && l < lev());

            return ATTR_W | ATTR_R | l << 9 | ATTR_A | ATTR_P;
        }

        // PTE attributes for LEAF at level l with [11:9] == 0
        static constexpr auto attr_leaf (unsigned, Paging::Permissions p, Memattr)
        {
            return !(p & (Paging::W | Paging::R)) ? 0 :
                     ATTR_W * !!(p & Paging::W) |
                     ATTR_R * !!(p & Paging::R) |
                     ATTR_D | ATTR_A | ATTR_P;
        }

        constexpr auto page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                      !!(val & ATTR_W) * Paging::W |
                                      !!(val & ATTR_R) * Paging::R);
        }

        static constexpr auto page_ma (unsigned) { return Memattr {}; }

        explicit constexpr Pte_dma_amd() = default;
        explicit constexpr Pte_dma_amd (Entry e) : Pte { e } {}
};

struct Ptab_dma_amd final : Ptab<Pte_dma_amd, uint64_t, uint64_t>
{
    explicit constexpr Ptab_dma_amd() : Ptab { Pte_dma_amd { 0 } } {}

    ~Ptab_dma_amd() { deallocate (&root); }
};

// Sanity Checks
static_assert (Pte_dma_amd::lev() == 5);
static_assert (Pte_dma_amd::lev_bit (0) == 9 && Pte_dma_amd::lev_bit (1) == 9 && Pte_dma_amd::lev_bit (2) == 9 && Pte_dma_amd::lev_bit (3) == 9 && Pte_dma_amd::lev_bit (4) == 9);
static_assert (Pte_dma_amd::lev_idx (0, BITN (Pte_dma_amd::ibits) - 1) == 511 && Pte_dma_amd::lev_idx (1, BITN (Pte_dma_amd::ibits) - 1) == 511 && Pte_dma_amd::lev_idx (2, BITN (Pte_dma_amd::ibits) - 1) == 511 && Pte_dma_amd::lev_idx (3, BITN (Pte_dma_amd::ibits) - 1) == 511 && Pte_dma_amd::lev_idx (4, BITN (Pte_dma_amd::ibits) - 1) == 511);
