/*
 * Extended Page Table (EPT)
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

#include "assert.hpp"
#include "pte.hpp"

class Ept : public Pte<Ept, 4, 9, uint64, uint64>
{
    friend class Pte;

    using Pte::Pte;

    public:
        static inline bool mbec { false };

    protected:
        static constexpr OAddr ADDR_MASK = BIT64_RANGE (51, 12);

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
            ATTR_SSS    = BIT64 (60),   // Supervisor Shadow Stack
            ATTR_SPP    = BIT64 (61),   // Subpage Write Permissions
            ATTR_SVE    = BIT64 (63),   // Suppress #VE
        };

        bool large (unsigned l) const { return l &&  (val & ATTR_S); }
        bool table (unsigned l) const { return l && !(val & ATTR_S); }

        // Attributes for PTEs referring to page tables
        static OAddr ptab_attr()
        {
            return ATTR_XU | ATTR_XS | ATTR_W | ATTR_R;
        }

        // Attributes for PTEs referring to leaf pages
        static OAddr page_attr (unsigned l, Paging::Permissions pm, Memattr::Cacheability ca, Memattr::Shareability)
        {
            return !(pm & Paging::API) ? 0 :
                     ATTR_XS * !!(pm & (mbec ? Paging::XS : Paging::XS | Paging::XU)) |
                     ATTR_XU * !!(pm & (mbec ? Paging::XU : Paging::XS | Paging::XU)) |
                     ATTR_W  * !!(pm & Paging::W) |
                     ATTR_R  * !!(pm & Paging::R) |
                     ATTR_S  * !!l | Memattr::ca_to_ept (ca) << 3;
        }

        static Paging::Permissions page_pm (OAddr a)
        {
            return Paging::Permissions (!a ? 0 :
                                      !!(a & ATTR_XS) * Paging::XS |
                                      !!(a & ATTR_XU) * Paging::XU |
                                      !!(a & ATTR_W)  * Paging::W  |
                                      !!(a & ATTR_R)  * Paging::R);
        }

        static Memattr::Cacheability page_ca (OAddr a, unsigned) { return Memattr::ept_to_ca (a >> 3 & 0x7); }

        static Memattr::Shareability page_sh (OAddr) { return Memattr::Shareability::NONE; }
};

class Eptp final : public Ept
{
    public:
        ALWAYS_INLINE
        inline void flush()
        {
            struct { uint64 eptp, rsvd; } desc = { addr() | (lev - 1) << 3 | 6, 0 };

            bool ret;
            asm volatile ("invept %1, %2" : "=@cca" (ret) : "m" (desc), "r" (1UL) : "memory");
            assert (ret);
        }
};
