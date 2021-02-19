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

#include "pte.hpp"

class Dpt : public Pte<Dpt, 4, 9, uint64, uint64>
{
    friend class Pte;

    using Pte::Pte;

    private:
        enum
        {
            ATTR_R      = BIT64  (0),   // Readable
            ATTR_W      = BIT64  (1),   // Writable
            ATTR_S      = BIT64  (7),   // Superpage
        };

        bool large (unsigned l) const { return l &&  (val & ATTR_S); }
        bool table (unsigned l) const { return l && !(val & ATTR_S); }

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
        static Paging::Permissions page_pm (OAddr a)
        {
            return Paging::Permissions (!a ? 0 :
                                      !!(a & ATTR_W) * Paging::W |
                                      !!(a & ATTR_R) * Paging::R);
        }

        // FIXME
        static Memattr::Cacheability page_ca (OAddr, unsigned) { return Memattr::Cacheability::MEM_WB; }

        // FIXME
        static Memattr::Shareability page_sh (OAddr) { return Memattr::Shareability::NONE; }

    protected:
        static constexpr OAddr ADDR_MASK = BIT64_RANGE (51, 12);
};

class Dptp final : public Dpt
{
    public:
        ALWAYS_INLINE
        inline void flush()
        {
        }
};
