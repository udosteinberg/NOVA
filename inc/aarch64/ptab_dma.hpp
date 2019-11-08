/*
 * DMA Page Table (Arm SMMU Format)
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

class Pte_dma final : public Pte<Pte_dma, uint64_t, uint64_t>
{
    friend class Pte;

    private:
        enum
        {
            ATTR_OWA    = BITN (63),    // Override Write-Allocate
            ATTR_ORA    = BITN (61),    // Override Read-Allocate
            ATTR_A      = BITN (10),    // Accessed
            ATTR_W      = BITN  (7),    // Writable
            ATTR_R      = BITN  (6),    // Readable
            ATTR_nL     = BITN  (1),    // Not Large
            ATTR_P      = BITN  (0),    // Present
        };

        static_assert (!(DEAD_MARKER & ATTR_P));

    public:
        static constexpr unsigned ibits { 48 };

        static inline constinit bool noncoherent { false };

        // PTE attributes for PTAB at level l
        static constexpr auto attr_ptab (unsigned l)
        {
            assert (0 < l && l < lev());

            return ATTR_nL | ATTR_P;
        }

        // PTE attributes for LEAF at level l
        static constexpr auto attr_leaf (unsigned l, Paging::Permissions p, Memattr a)
        {
            return !(p & Paging::API) ? 0 :
                     ATTR_W * !!(p & Paging::W) |
                     ATTR_R * !!(p & Paging::R) |
                     a.share() << 8 | a.cache_s2() << 2 | !l * ATTR_nL | ATTR_A | ATTR_P;
        }

        constexpr auto page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                      !!(val & ATTR_W) * Paging::W |
                                      !!(val & ATTR_R) * Paging::R);
        }

        constexpr auto page_ma (unsigned) const
        {
            return Memattr { Memattr::Share (BIT_RANGE (1, 0) & val >> 8),
                             Memattr::Cache (!!(BIT_RANGE (5, 4) & val) * 4 + (BIT_RANGE (1, 0) & val >> 2)) };
        }

        explicit constexpr Pte_dma() = default;
        explicit constexpr Pte_dma (Entry e) : Pte { e } {}
};

struct Ptab_dma final : Ptab<Pte_dma, uint64_t, uint64_t>
{
    explicit constexpr Ptab_dma() : Ptab { Pte_dma { 0 } } {}

    ~Ptab_dma() { deallocate (&root); }
};

// Sanity Checks
static_assert (Pte_dma::lev() == 4);
static_assert (Pte_dma::lev_bit (0) == 9 && Pte_dma::lev_bit (1) == 9 && Pte_dma::lev_bit (2) == 9 && Pte_dma::lev_bit (3) == 9);
static_assert (Pte_dma::lev_idx (0, BITN (Pte_dma::ibits) - 1) == 511 && Pte_dma::lev_idx (1, BITN (Pte_dma::ibits) - 1) == 511 && Pte_dma::lev_idx (2, BITN (Pte_dma::ibits) - 1) == 511 && Pte_dma::lev_idx (3, BITN (Pte_dma::ibits) - 1) == 511);
