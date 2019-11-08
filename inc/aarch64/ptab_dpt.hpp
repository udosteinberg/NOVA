/*
 * DMA Page Table (DPT)
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

class Dpt final : public Pte<Dpt, uint64_t, uint64_t>
{
    friend class Pte;

    private:
        enum
        {
            ATTR_P      = BIT64  (0),   // Present
            ATTR_nL     = BIT64  (1),   // Not Large
            ATTR_R      = BIT64  (6),   // Readable
            ATTR_W      = BIT64  (7),   // Writable
            ATTR_A      = BIT64 (10),   // Accessed
            ATTR_ORA    = BIT64 (61),   // Override Read-Allocate
            ATTR_OWA    = BIT64 (63),   // Override Write-Allocate
        };

    public:
        static constexpr unsigned ibits { 48 };
        static constexpr auto ptab_attr { ATTR_nL | ATTR_P };

        static inline bool noncoherent { false };

        // Attributes for PTEs referring to leaf pages
        static OAddr page_attr (unsigned l, Paging::Permissions p, Memattr a)
        {
            return !(p & Paging::API) ? 0 :
                     ATTR_W * !!(p & Paging::W) |
                     ATTR_R * !!(p & Paging::R) |
                     a.share() << 8 | a.cache_s2() << 2 | !l * ATTR_nL | ATTR_A | ATTR_P;
        }

        auto page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                      !!(val & ATTR_W) * Paging::W |
                                      !!(val & ATTR_R) * Paging::R);
        }

        auto page_ma (unsigned) const
        {
            return Memattr { Memattr::Share (BIT_RANGE (1, 0) & val >> 8),
                             Memattr::Cache (!!(BIT_RANGE (5, 4) & val) * 4 + (BIT_RANGE (1, 0) & val >> 2)) };
        }

        Dpt() = default;
        Dpt (Entry e) : Pte (e) {}
};

class Dptp final : public Ptab<Dpt, uint64_t, uint64_t>
{
    public:
        explicit Dptp (OAddr v = 0) : Ptab (Dpt (v)) {}
};

// Sanity checks
static_assert (Dpt::lev() == 4);
static_assert (Dpt::lev_bit (0) == 9 && Dpt::lev_bit (1) == 9 && Dpt::lev_bit (2) == 9 && Dpt::lev_bit (3) == 9);
static_assert (Dpt::lev_idx (0, BITN (Dpt::ibits) - 1) == 511 && Dpt::lev_idx (1, BITN (Dpt::ibits) - 1) == 511 && Dpt::lev_idx (2, BITN (Dpt::ibits) - 1) == 511 && Dpt::lev_idx (3, BITN (Dpt::ibits) - 1) == 511);
