/*
 * Guest Page Table (AMD Stage-2 / LPT Format)
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

class Pte_gst_amd final : public Pte_lpt<Pte_gst_amd, uint64_t, uint64_t>
{
    friend class Pte;

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
                     ATTR_nX *  !(p & (Paging::XS | Paging::XU))        |
                     ATTR_W  * !!(p &  Paging::W)                       |
                     ATTR_S  * !!l | ATTR_D | ATTR_A | ATTR_U | ATTR_P  |
                     attr_ma (l, a);
        }

        constexpr auto page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                       !(val & ATTR_nX) * (Paging::XS | Paging::XU) |
                                      !!(val & ATTR_W)  *  Paging::W                |
                                      !!(val & ATTR_P)  *  Paging::R);
        }

        explicit constexpr Pte_gst_amd() = default;
        explicit constexpr Pte_gst_amd (Entry e) : Pte_lpt { e } {}
};

struct Ptab_gst_amd final : Ptab<Pte_gst_amd, uint64_t, uint64_t>
{
    explicit constexpr Ptab_gst_amd() : Ptab { Pte_gst_amd { 0 } } {}

    ~Ptab_gst_amd() { deallocate (&root); }

    // Compute full hardware nCR3 value
    [[nodiscard]] auto root_hw() const { return root_addr(); }
};

// Sanity Checks
static_assert (Pte_gst_amd::lev() == 4);
static_assert (Pte_gst_amd::lev_bit (0) == 9 && Pte_gst_amd::lev_bit (1) == 9 && Pte_gst_amd::lev_bit (2) == 9 && Pte_gst_amd::lev_bit (3) == 9);
static_assert (Pte_gst_amd::lev_idx (0, BITN (Pte_gst_amd::ibits) - 1) == 511 && Pte_gst_amd::lev_idx (1, BITN (Pte_gst_amd::ibits) - 1) == 511 && Pte_gst_amd::lev_idx (2, BITN (Pte_gst_amd::ibits) - 1) == 511 && Pte_gst_amd::lev_idx (3, BITN (Pte_gst_amd::ibits) - 1) == 511);
