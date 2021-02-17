/*
 * DMA Page Table (DPT)
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

class Dpt final : public Pte<Dpt, uint64, uint64>
{
    friend class Pte;

    private:
        enum
        {
            ATTR_R      = BIT64  (0),   // Readable
            ATTR_W      = BIT64  (1),   // Writable
            ATTR_S      = BIT64  (7),   // Superpage
        };

    public:
        static constexpr unsigned ibits { 48 };
        static constexpr auto ptab_attr { ATTR_W | ATTR_R };

        static inline bool noncoherent { false };

        // Attributes for PTEs referring to leaf pages
        static OAddr page_attr (unsigned l, Paging::Permissions p, Memattr a)
        {
            return !(p & (Paging::W | Paging::R)) ? 0 :
                     ATTR_W * !!(p & Paging::W) |
                     ATTR_R * !!(p & Paging::R) |
                     ATTR_S * !!l               |
                     static_cast<OAddr>(a.keyid()) << Memattr::obits | a.cache_s2() << 3;
        }

        auto page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                      !!(val & ATTR_W) * Paging::W |
                                      !!(val & ATTR_R) * Paging::R);
        }

        auto page_ma (unsigned) const
        {
            return Memattr { Memattr::Keyid (val >> Memattr::obits & (BIT (Memattr::kbits) - 1)),
                             Memattr::ept_to_ca (val >> 3 & BIT_RANGE (2, 0)) };
        }

        Dpt() = default;
        Dpt (Entry e) : Pte (e) {}
};

class Dptp final : public Ptab<Dpt, uint64, uint64>
{
    public:
        explicit Dptp (OAddr v = 0) : Ptab (Dpt (v)) {}
};

// Sanity checks
static_assert (Dpt::lev() == 4);
static_assert (Dpt::lev_bit (0) == 9 && Dpt::lev_bit (1) == 9 && Dpt::lev_bit (2) == 9 && Dpt::lev_bit (3) == 9);
static_assert (Dpt::lev_idx (0, BITN (Dpt::ibits) - 1) == 511 && Dpt::lev_idx (1, BITN (Dpt::ibits) - 1) == 511 && Dpt::lev_idx (2, BITN (Dpt::ibits) - 1) == 511 && Dpt::lev_idx (3, BITN (Dpt::ibits) - 1) == 511);
