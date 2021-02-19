/*
 * DMA Page Table (Intel IOMMU Format)
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

class Pte_dma_itl final : public Pte_ept<Pte_dma_itl, uint64_t, uint64_t>
{
    friend class Pte;

    private:
        enum
        {
            ATTR_IW     = BITN (62),    // I/O Write
            ATTR_IR     = BITN (61),    // I/O Read
            ATTR_SNP    = BITN (11),    // Snoop
        };

    public:
        static constexpr unsigned ibits { 57 };

        // TLB invalidation requirements for Intel IOMMU
        static        constexpr bool inv_upgrade    { true  };  // Any PTE change requires invalidation
        static        constexpr bool inv_splinter   { true  };  // Any PTE change requires invalidation
        static inline constinit bool inv_notpresent { false };  // True if SMMU caches not-present entries
        static inline constinit bool noncoherent    { false };

        // PTE attributes for PTAB at level l
        static constexpr auto attr_ptab (unsigned l)
        {
            assert (0 < l && l < lev());

            return ATTR_W | ATTR_R;
        }

        // PTE attributes for LEAF at level l
        static auto attr_leaf (unsigned l, Paging::Permissions p, Memattr a)
        {
            return !(p & (Paging::W | Paging::R)) ? 0 :
                     ATTR_W * !!(p & Paging::W) |
                     ATTR_R * !!(p & Paging::R) |
                     ATTR_S * !!l               |
                     attr_ma (a);
        }

        constexpr auto page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                      !!(val & ATTR_W) * Paging::W |
                                      !!(val & ATTR_R) * Paging::R);
        }

        explicit constexpr Pte_dma_itl() = default;
        explicit constexpr Pte_dma_itl (Entry e) : Pte_ept { e } {}
};

struct Ptab_dma_itl final : Ptab<Pte_dma_itl, uint64_t, uint64_t>
{
    explicit constexpr Ptab_dma_itl() : Ptab { Pte_dma_itl { 0 } } {}

    ~Ptab_dma_itl() { deallocate (&root); }
};

// Sanity Checks
static_assert (Pte_dma_itl::lev() == 5);
static_assert (Pte_dma_itl::lev_bit (0) == 9 && Pte_dma_itl::lev_bit (1) == 9 && Pte_dma_itl::lev_bit (2) == 9 && Pte_dma_itl::lev_bit (3) == 9 && Pte_dma_itl::lev_bit (4) == 9);
static_assert (Pte_dma_itl::lev_idx (0, BITN (Pte_dma_itl::ibits) - 1) == 511 && Pte_dma_itl::lev_idx (1, BITN (Pte_dma_itl::ibits) - 1) == 511 && Pte_dma_itl::lev_idx (2, BITN (Pte_dma_itl::ibits) - 1) == 511 && Pte_dma_itl::lev_idx (3, BITN (Pte_dma_itl::ibits) - 1) == 511 && Pte_dma_itl::lev_idx (4, BITN (Pte_dma_itl::ibits) - 1) == 511);
