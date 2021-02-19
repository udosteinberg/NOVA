/*
 * Extended Page Table (EPT)
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

class Ept final : public Pte<Ept, uint64_t, uint64_t>
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
            ATTR_XU     = BITN (10),    // Executable (User)
            ATTR_D      = BITN  (9),    // Dirty
            ATTR_A      = BITN  (8),    // Accessed
            ATTR_S      = BITN  (7),    // Superpage
            ATTR_I      = BITN  (6),    // Ignore PAT
            ATTR_XS     = BITN  (2),    // Executable (Supervisor)
            ATTR_W      = BITN  (1),    // Writable
            ATTR_R      = BITN  (0),    // Readable
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
        static constexpr auto attr_leaf (unsigned l, Paging::Permissions p, Memattr a)
        {
            return !(p & Paging::API) ? 0 :
                     ATTR_XS * !!(p & (mbec ? Paging::XS : Paging::XS | Paging::XU)) |
                     ATTR_XU * !!(p & (mbec ? Paging::XU : Paging::XS | Paging::XU)) |
                     ATTR_W  * !!(p & Paging::W)    |
                     ATTR_R  * !!(p & Paging::R)    |
                     ATTR_S  * !!l                  |
                     a.key_encode() | a.cache_s2() << 3;
        }

        auto page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                      !!(val & ATTR_XS) * Paging::XS |
                                      !!(val & ATTR_XU) * Paging::XU |
                                      !!(val & ATTR_W)  * Paging::W  |
                                      !!(val & ATTR_R)  * Paging::R);
        }

        auto page_ma (unsigned) const
        {
            return Memattr { Memattr::ept_to_ca (val >> 3 & BIT_RANGE (2, 0)), Memattr::key_decode (val) };
        }

        explicit constexpr Ept() = default;
        explicit constexpr Ept (Entry e) : Pte { e } {}
};

struct Eptp final : Ptab<Ept, uint64_t, uint64_t>
{
    explicit constexpr Eptp() : Ptab { Ept { 0 } } {}

    bool invalidate() const
    {
        uint128_t desc { root_addr() };     // INVEPT only uses EPTP[51:12]

        bool ret;
        asm volatile ("invept %1, %2" : "=@cca" (ret) : "m" (desc), "r" (1UL) : "memory");
        return ret;
    }
};

// Sanity checks
static_assert (Ept::lev() == 4);
static_assert (Ept::lev_bit (0) == 9 && Ept::lev_bit (1) == 9 && Ept::lev_bit (2) == 9 && Ept::lev_bit (3) == 9);
static_assert (Ept::lev_idx (0, BITN (Ept::ibits) - 1) == 511 && Ept::lev_idx (1, BITN (Ept::ibits) - 1) == 511 && Ept::lev_idx (2, BITN (Ept::ibits) - 1) == 511 && Ept::lev_idx (3, BITN (Ept::ibits) - 1) == 511);
