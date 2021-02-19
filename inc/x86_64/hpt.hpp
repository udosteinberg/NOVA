/*
 * Host Page Table (HPT)
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

class Hpt : public Pte<Hpt, 4, 9, uint64, uint64>
{
    friend class Pte;

    using Pte::Pte;

    private:
        enum
        {
            ATTR_P      = BIT64  (0),   // Present
            ATTR_W      = BIT64  (1),   // Writable
            ATTR_U      = BIT64  (2),   // User
            ATTR_A      = BIT64  (5),   // Accessed
            ATTR_D      = BIT64  (6),   // Dirty
            ATTR_S      = BIT64  (7),   // Superpage
            ATTR_G      = BIT64  (8),   // Global
            ATTR_K      = BIT64  (9),   // Kernel Memory
            ATTR_nX     = BIT64 (63),   // Not Executable
        };

        bool large (unsigned l) const { return l &&  (val & ATTR_S); }
        bool table (unsigned l) const { return l && !(val & ATTR_S); }

        // Attributes for PTEs referring to page tables
        static OAddr ptab_attr()
        {
            return ATTR_A | ATTR_U | ATTR_W | ATTR_P;
        }

        // Attributes for PTEs referring to leaf pages
        static OAddr page_attr (unsigned l, Paging::Permissions pm, Memattr::Cacheability ca, Memattr::Shareability)
        {
            return !(pm & Paging::API) ? 0 :
                     ATTR_G  * !!(pm &  Paging::G)                  |
                     ATTR_K  * !!(pm &  Paging::K)                  |
                     ATTR_U  * !!(pm &  Paging::U)                  |
                     ATTR_nX *  !(pm & (Paging::XS | Paging::XU))   |
                     ATTR_W  * !!(pm &  Paging::W)                  |
                     ATTR_S  * !!l | ATTR_D | ATTR_A | ATTR_P       |
                     (static_cast<OAddr>(ca) & BIT (2)) << (l ? 10 : 5) | (static_cast<OAddr>(ca) & BIT_RANGE (1, 0)) << 3;
        }

        static Paging::Permissions page_pm (OAddr a)
        {
            return Paging::Permissions (!a ? 0 :
                                      !!(a & ATTR_G)  *  Paging::G                  |
                                      !!(a & ATTR_K)  *  Paging::K                  |
                                      !!(a & ATTR_U)  *  Paging::U                  |
                                       !(a & ATTR_nX) * (Paging::XS | Paging::XU)   |
                                      !!(a & ATTR_W)  *  Paging::W                  |
                                      !!(a & ATTR_P)  *  Paging::R);
        }

        static Memattr::Cacheability page_ca (OAddr a, unsigned l) { return Memattr::Cacheability ((a >> (l ? 10 : 5) & BIT (2)) | (a >> 3 & BIT_RANGE (1, 0))); }

        static Memattr::Shareability page_sh (OAddr) { return Memattr::Shareability::NONE; }

    protected:
        static constexpr OAddr ADDR_MASK = static_cast<OAddr>(BIT64_RANGE (51, 12));

    public:
        // FIXME below
        bool sync_from (Hpt, mword, mword);

        void sync_master_range (mword, mword);

        Paddr replace (mword, mword, bool);

        static void *map (OAddr, bool = false);
};

class Hptp final : public Hpt
{
    public:
        explicit Hptp (OAddr v = 0) { val = v; }

        ALWAYS_INLINE
        static inline Hptp current()
        {
            OAddr val;
            asm volatile ("mov %%cr3, %0" : "=r" (val));
            return Hptp (val & ADDR_MASK);
        }

        ALWAYS_INLINE
        inline void make_current (uintptr_t pcid)
        {
            assert (!(val & ~ADDR_MASK));

            asm volatile ("mov %0, %%cr3" : : "r" (val | pcid) : "memory");
        }

        ALWAYS_INLINE
        static inline void flush()
        {
            uintptr_t cr3;
            asm volatile ("mov %%cr3, %0; mov %0, %%cr3" : "=&r" (cr3));
        }

        ALWAYS_INLINE
        static inline void flush (uintptr_t addr)
        {
            asm volatile ("invlpg %0" : : "m" (*reinterpret_cast<uintptr_t *>(addr)));
        }
};
