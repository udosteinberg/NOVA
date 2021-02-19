/*
 * Host Page Table (HPT)
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

class Hpt final : public Pte<Hpt, uint64, uint64>
{
    friend class Pte;

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

    public:
        static constexpr unsigned ibits { 48 };
        static constexpr auto ptab_attr { ATTR_A | ATTR_U | ATTR_W | ATTR_P };

        // Attributes for PTEs referring to leaf pages
        static OAddr page_attr (unsigned l, Paging::Permissions p, Memattr a)
        {
            auto const cache { a.cache_s1() };

            return !(p & Paging::API) ? 0 :
                     ATTR_D  * !!(p & (Paging::SS | Paging::W))         |
                     ATTR_G  * !!(p &  Paging::G)                       |
                     ATTR_K  * !!(p &  Paging::K)                       |
                     ATTR_U  * !!(p &  Paging::U)                       |
                     ATTR_nX *  !(p & (Paging::XS | Paging::XU))        |
                     ATTR_W  * !!(p &  Paging::W)                       |
                     ATTR_S  * !!l | ATTR_A | ATTR_P                    |
                     static_cast<OAddr>(a.keyid()) << Memattr::obits    |
                     (cache & BIT (2)) << (l ? 10 : 5) | (cache & BIT_RANGE (1, 0)) << 3;
        }

        auto page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                      !!(val & ATTR_G)  *  Paging::G                |
                                      !!(val & ATTR_K)  *  Paging::K                |
                                      !!(val & ATTR_U)  *  Paging::U                |
                                       !(val & ATTR_nX) * (Paging::XS | Paging::XU) |
                                      !!(val & ATTR_W)  *  Paging::W                |
                                      !!(val & ATTR_P)  *  Paging::R);
        }

        auto page_ma (unsigned l) const
        {
            return Memattr { Memattr::Keyid (val >> Memattr::obits & (BIT (Memattr::kbits) - 1)),
                             Memattr::Cache ((val >> (l ? 10 : 5) & BIT (2)) | (val >> 3 & BIT_RANGE (1, 0))) };
        }

        Hpt() = default;
        Hpt (Entry e) : Pte (e) {}
};

class Hptp final : public Ptab<Hpt, uint64, uint64>
{
    friend class Pd;

    private:
        static Hptp master;

    public:
        explicit Hptp (OAddr v = 0) : Ptab (Hpt (v)) {}

        // Copy Constructor
        Hptp (Hptp const &x) : Ptab (static_cast<Hpt>(x.entry)) {}

        // Copy Assignment
        Hptp& operator= (Hptp const &x)
        {
            entry = static_cast<Hpt>(x.entry);

            return *this;
        }

        ALWAYS_INLINE
        static inline Hptp current()
        {
            OAddr val;
            asm volatile ("mov %%cr3, %0" : "=r" (val));
            return Hptp (val & Hpt::addr_mask());
        }

        ALWAYS_INLINE
        inline void make_current (uintptr_t pcid) const
        {
            asm volatile ("mov %0, %%cr3" : : "r" (root_addr() | pcid) : "memory");
        }

        ALWAYS_INLINE
        static inline void invalidate()
        {
            uintptr_t cr3;
            asm volatile ("mov %%cr3, %0; mov %0, %%cr3" : "=&r" (cr3));
        }

        ALWAYS_INLINE
        static inline void invalidate (uintptr_t addr)
        {
            asm volatile ("invlpg %0" : : "m" (*reinterpret_cast<uintptr_t *>(addr)));
        }

        ALWAYS_INLINE
        static inline void master_map (IAddr v, OAddr p, unsigned o, Paging::Permissions pm, Memattr ma)
        {
            master.update (v, p, o, pm, ma);
        }

        ALWAYS_INLINE
        inline bool share_from_master (IAddr v)
        {
            return share_from (master, v, MMAP_CPU);
        }

        ALWAYS_INLINE
        inline void share_from_master (IAddr s, IAddr e)
        {
            for (unsigned l = (bit_scan_reverse (LINK_ADDR ^ MMAP_CPU) - PAGE_BITS) / Hpt::bpl; s < e; s += BITN (l * Hpt::bpl + PAGE_BITS))
                share_from_master (s);
        }

        bool share_from (Hptp, IAddr, IAddr);

        static void *map (OAddr, bool = false);
};

// Sanity checks
static_assert (Hpt::lev() == 4);
static_assert (Hpt::lev_bit (0) == 9 && Hpt::lev_bit (1) == 9 && Hpt::lev_bit (2) == 9 && Hpt::lev_bit (3) == 9);
static_assert (Hpt::lev_idx (0, BITN (Hpt::ibits) - 1) == 511 && Hpt::lev_idx (1, BITN (Hpt::ibits) - 1) == 511 && Hpt::lev_idx (2, BITN (Hpt::ibits) - 1) == 511 && Hpt::lev_idx (3, BITN (Hpt::ibits) - 1) == 511);
