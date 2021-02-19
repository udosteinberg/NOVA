/*
 * DMA Page Table (DPT)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#include "ptab.hpp"

class Dpt final : public Pagetable<Dpt, uint64, uint64, 4, 3, true>::Entry
{
    private:
        enum
        {
            ATTR_R      = BIT64  (0),   // Readable
            ATTR_W      = BIT64  (1),   // Writable
            ATTR_S      = BIT64  (7),   // Superpage
        };

    public:
        static constexpr OAddr ADDR_MASK = BIT64_RANGE (51, 12);

        bool is_large (unsigned l) const { return l &&  (val & ATTR_S); }
        bool is_table (unsigned l) const { return l && !(val & ATTR_S); }

        // Attributes for PTEs referring to page tables
        static OAddr ptab_attr()
        {
            return ATTR_W | ATTR_R;
        }

        // FIXME: Attributes for PTEs referring to leaf pages
        static OAddr page_attr (unsigned l, Paging::Permissions pm, Memattr::Cacheability, Memattr::Shareability)
        {
            return !(pm & (Paging::W | Paging::R)) ? 0 :
                     ATTR_W * !!(pm & Paging::W) |
                     ATTR_R * !!(pm & Paging::R) |
                     ATTR_S * !!l;
        }

        // FIXME
        Paging::Permissions page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                      !!(val & ATTR_W) * Paging::W |
                                      !!(val & ATTR_R) * Paging::R);
        }

        // FIXME
        Memattr::Cacheability page_ca (unsigned) const { return Memattr::Cacheability::MEM_WB; }

        // FIXME
        Memattr::Shareability page_sh() const { return Memattr::Shareability::NONE; }

        // Needed by gcc version < 10
        ALWAYS_INLINE inline Dpt() : Entry() {}
        ALWAYS_INLINE inline Dpt (Entry x) : Entry (x) {}
};

class Dptp final : public Pagetable<Dpt, uint64, uint64, 4, 3, true>
{
    public:
        // Constructor
        ALWAYS_INLINE
        inline explicit Dptp (OAddr v = 0) : Pagetable (Dpt (v)) {}

        // FIXME
        ALWAYS_INLINE
        inline void invalidate() const {}
};
