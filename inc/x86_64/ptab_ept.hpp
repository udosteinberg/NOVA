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

#include "ptab.hpp"

class Ept final : public Pagetable<Ept, uint64, uint64, 4, 3, false>::Entry
{
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
        static inline bool mbec { true };

        static constexpr OAddr ADDR_MASK { BIT64_RANGE (51, 12) };

        bool is_large (unsigned l) const { return l &&  (val & ATTR_S); }
        bool is_table (unsigned l) const { return l && !(val & ATTR_S); }

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

        Paging::Permissions page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                      !!(val & ATTR_XS) * Paging::XS |
                                      !!(val & ATTR_XU) * Paging::XU |
                                      !!(val & ATTR_W)  * Paging::W  |
                                      !!(val & ATTR_R)  * Paging::R);
        }

        Memattr::Cacheability page_ca (unsigned) const { return Memattr::ept_to_ca (val >> 3 & 0x7); }

        Memattr::Shareability page_sh() const { return Memattr::Shareability::NONE; }

        // Needed by gcc version < 10
        ALWAYS_INLINE inline Ept() : Entry() {}
        ALWAYS_INLINE inline Ept (Entry x) : Entry (x) {}
};

class Eptp final : public Pagetable<Ept, uint64, uint64, 4, 3, false>
{
    public:
        // Constructor
        ALWAYS_INLINE
        inline explicit Eptp (OAddr v = 0) : Pagetable (Ept (v)) {}

        ALWAYS_INLINE
        inline void invalidate() const
        {
            struct { uint64 eptp, rsvd; } desc = { root_addr() | (lev - 1) << 3 | 6, 0 };

            bool ret;
            asm volatile ("invept %1, %2" : "=@cca" (ret) : "m" (desc), "r" (1UL) : "memory");
            assert (ret);
        }
};
