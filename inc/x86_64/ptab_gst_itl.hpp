/*
 * Guest Page Table (Intel Stage-2 / EPT Format)
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

class Pte_gst_itl final : public Pte_ept<Pte_gst_itl, uint64_t, uint64_t>
{
    friend class Pte;

    private:
        enum
        {
            ATTR_SVE    = BITN (63),    // Suppress #VE
            ATTR_SPP    = BITN (61),    // Subpage Write Permissions
            ATTR_SSS    = BITN (60),    // Supervisor Shadow Stack
            ATTR_PW     = BITN (58),    // Paging-Write Access
            ATTR_VGP    = BITN (57),    // Verify Guest Paging
        };

    public:
        static constexpr unsigned ibits { 48 };

        static inline constinit bool mbec { true };

        // PTE attributes for PTAB at level l
        static constexpr auto attr_ptab (unsigned l)
        {
            assert (0 < l && l < lev());

            return ATTR_XU | ATTR_XS | ATTR_W | ATTR_R;
        }

        // PTE attributes for LEAF at level l
        static auto attr_leaf (unsigned l, Paging::Permissions p, Memattr a)
        {
            return !(p & Paging::API) ? 0 :
                     ATTR_XS * !!(p & (mbec ? Paging::XS : Paging::XS | Paging::XU)) |
                     ATTR_XU * !!(p & (mbec ? Paging::XU : Paging::XS | Paging::XU)) |
                     ATTR_W  * !!(p & Paging::W) |
                     ATTR_R  * !!(p & (Paging::W | Paging::R)) |        // W implies R
                     ATTR_S  * !!l |
                     attr_ma (a);
        }

        constexpr auto page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                      !!(val & ATTR_XS) * Paging::XS |
                                      !!(val & ATTR_XU) * Paging::XU |
                                      !!(val & ATTR_W)  * Paging::W  |
                                      !!(val & ATTR_R)  * Paging::R);
        }

        explicit constexpr Pte_gst_itl() = default;
        explicit constexpr Pte_gst_itl (Entry e) : Pte_ept { e } {}
};

struct Ptab_gst_itl final : Ptab<Pte_gst_itl, uint64_t, uint64_t>
{
    explicit constexpr Ptab_gst_itl() : Ptab { Pte_gst_itl { 0 } } {}

    ~Ptab_gst_itl() { deallocate (&root); }

    // Compute full hardware EPTP value
    [[nodiscard]] auto root_hw() const { return root_addr() | (Pte_gst_itl::lev() - 1) << 3 | CA_TYPE_MEM_WB; }

    // Invalidate TLB entries derived from this page table
    [[nodiscard]] bool invalidate() const
    {
        // Single-context INVEPT requires a full EPTP conforming to VM entry checks; global INVEPT ignores it
        uint128_t const desc { root_hw() };

        bool ret;
        asm volatile ("invept %1, %2" : "=@cca" (ret) : "m" (desc), "r" (1UL) : "memory");
        return ret;
    }
};

// Sanity Checks
static_assert (Pte_gst_itl::lev() == 4);
static_assert (Pte_gst_itl::lev_bit (0) == 9 && Pte_gst_itl::lev_bit (1) == 9 && Pte_gst_itl::lev_bit (2) == 9 && Pte_gst_itl::lev_bit (3) == 9);
static_assert (Pte_gst_itl::lev_idx (0, BITN (Pte_gst_itl::ibits) - 1) == 511 && Pte_gst_itl::lev_idx (1, BITN (Pte_gst_itl::ibits) - 1) == 511 && Pte_gst_itl::lev_idx (2, BITN (Pte_gst_itl::ibits) - 1) == 511 && Pte_gst_itl::lev_idx (3, BITN (Pte_gst_itl::ibits) - 1) == 511);
