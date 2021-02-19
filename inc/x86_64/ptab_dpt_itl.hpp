/*
 * DMA Page Table (DPT): Intel IOMMU
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

class Dpt_itl final : public Pte<Dpt_itl, uint64_t, uint64_t>
{
    friend class Pte;

    private:
        enum
        {
            ATTR_S      = BITN  (7),    // Superpage
            ATTR_W      = BITN  (1),    // Writable
            ATTR_R      = BITN  (0),    // Readable
        };

    public:
        static constexpr unsigned ibits { 57 };

        // TLB invalidation policy override for Intel IOMMU
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
        static constexpr auto attr_leaf (unsigned l, Paging::Permissions p, Memattr a)
        {
            return !(p & (Paging::W | Paging::R)) ? 0 :
                     ATTR_W * !!(p & Paging::W) |
                     ATTR_R * !!(p & Paging::R) |
                     ATTR_S * !!l               |
                     a.key_encode() | a.cache_s2() << 3;
        }

        auto page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                      !!(val & ATTR_W) * Paging::W |
                                      !!(val & ATTR_R) * Paging::R);
        }

        auto page_ma (unsigned) const
        {
            return Memattr { Memattr::ept_to_ca (val >> 3 & BIT_RANGE (2, 0)), Memattr::key_decode (val) };
        }

        explicit constexpr Dpt_itl() = default;
        explicit constexpr Dpt_itl (Entry e) : Pte { e } {}
};

struct Dptp_itl final : Ptab<Dpt_itl, uint64_t, uint64_t>
{
    explicit constexpr Dptp_itl() : Ptab { Dpt_itl { 0 } } {}

    ~Dptp_itl() { deallocate_root (false); }
};

// Sanity checks
static_assert (Dpt_itl::lev() == 5);
static_assert (Dpt_itl::lev_bit (0) == 9 && Dpt_itl::lev_bit (1) == 9 && Dpt_itl::lev_bit (2) == 9 && Dpt_itl::lev_bit (3) == 9 && Dpt_itl::lev_bit (4) == 9);
static_assert (Dpt_itl::lev_idx (0, BITN (Dpt_itl::ibits) - 1) == 511 && Dpt_itl::lev_idx (1, BITN (Dpt_itl::ibits) - 1) == 511 && Dpt_itl::lev_idx (2, BITN (Dpt_itl::ibits) - 1) == 511 && Dpt_itl::lev_idx (3, BITN (Dpt_itl::ibits) - 1) == 511 && Dpt_itl::lev_idx (4, BITN (Dpt_itl::ibits) - 1) == 511);
