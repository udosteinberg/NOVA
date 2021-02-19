/*
 * Extended Page Table (EPT)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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

class Ept final : public Pte<Ept, uint64, uint64>
{
    friend class Pte;

    private:
        enum
        {
            ATTR_R      = BIT64  (0),   // Readable
            ATTR_W      = BIT64  (1),   // Writable
            ATTR_XS     = BIT64  (2),   // Executable (Supervisor)
            ATTR_I      = BIT64  (6),   // Ignore PAT
            ATTR_S      = BIT64  (7),   // Superpage
            ATTR_A      = BIT64  (8),   // Accessed
            ATTR_D      = BIT64  (9),   // Dirty
            ATTR_XU     = BIT64 (10),   // Executable (User)
            ATTR_VGP    = BIT64 (57),   // Verify Guest Paging
            ATTR_PW     = BIT64 (58),   // Paging-Write Access
            ATTR_SSS    = BIT64 (60),   // Supervisor Shadow Stack
            ATTR_SPP    = BIT64 (61),   // Subpage Write Permissions
            ATTR_SVE    = BIT64 (63),   // Suppress #VE
        };

    public:
        static constexpr unsigned ibits { 48 };
        static constexpr auto ptab_attr { ATTR_XU | ATTR_XS | ATTR_W | ATTR_R };

        static inline bool mbec { true };

        // Attributes for PTEs referring to leaf pages
        static OAddr page_attr (unsigned l, Paging::Permissions p, Memattr a)
        {
            return !(p & Paging::API) ? 0 :
                     ATTR_XS * !!(p & (mbec ? Paging::XS : Paging::XS | Paging::XU)) |
                     ATTR_XU * !!(p & (mbec ? Paging::XU : Paging::XS | Paging::XU)) |
                     ATTR_W  * !!(p & Paging::W)    |
                     ATTR_R  * !!(p & Paging::R)    |
                     ATTR_S  * !!l                  |
                     static_cast<OAddr>(a.keyid()) << Memattr::obits | a.cache_s2() << 3;
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
            return Memattr { Memattr::Keyid (val >> Memattr::obits & (BIT (Memattr::kbits) - 1)),
                             Memattr::ept_to_ca (val >> 3 & BIT_RANGE (2, 0)) };
        }

        Ept() = default;
        Ept (Entry e) : Pte (e) {}
};

class Eptp final : public Ptab<Ept, uint64, uint64>
{
    public:
        explicit Eptp (OAddr v = 0) : Ptab (Ept (v)) {}

        void invalidate() const
        {
            struct { uint64 eptp, rsvd; } desc = { root_addr() | (Ept::lev() - 1) << 3 | 6, 0 };

            bool ret;
            asm volatile ("invept %1, %2" : "=@cca" (ret) : "m" (desc), "r" (1UL) : "memory");
            assert (ret);
        }
};

// Sanity checks
static_assert (Ept::lev() == 4);
static_assert (Ept::lev_bit (0) == 9 && Ept::lev_bit (1) == 9 && Ept::lev_bit (2) == 9 && Ept::lev_bit (3) == 9);
static_assert (Ept::lev_idx (0, BITN (Ept::ibits) - 1) == 511 && Ept::lev_idx (1, BITN (Ept::ibits) - 1) == 511 && Ept::lev_idx (2, BITN (Ept::ibits) - 1) == 511 && Ept::lev_idx (3, BITN (Ept::ibits) - 1) == 511);
