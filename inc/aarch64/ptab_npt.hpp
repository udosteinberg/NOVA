/*
 * Nested Page Table (NPT)
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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
#include "vmid.hpp"

class Npt final : public Pte<Npt, uint64_t, uint64_t>
{
    friend class Pte;

    private:
        enum
        {
            ATTR_P      = BIT64  (0),   // Present
            ATTR_nL     = BIT64  (1),   // Not Large
            ATTR_R      = BIT64  (6),   // Readable
            ATTR_W      = BIT64  (7),   // Writable
            ATTR_A      = BIT64 (10),   // Accessed
            ATTR_nX0    = BIT64 (53),   // Not Executable
            ATTR_nX1    = BIT64 (54),   // Not Executable
            ATTR_K      = BIT64 (55),   // Kernel Memory
        };

    public:
        static constexpr unsigned ibits { 40 };
        static constexpr auto ptab_attr { ATTR_nL | ATTR_P };

        static inline bool xnx { true };

        static constexpr auto lev (unsigned b = ibits) { return (b - 4 - PAGE_BITS + bpl - 1) / bpl; }
        static constexpr auto lev_bit (unsigned l) { return l < lev() - 1 ? bpl : max (bpl, ibits - PAGE_BITS - l * bpl); }

        // Attributes for PTEs referring to leaf pages
        static OAddr page_attr (unsigned l, Paging::Permissions p, Memattr a)
        {
            bool const nxs { !(p & Paging::XS) };
            bool const nxu { !(p & Paging::XU) };

            return !(p & Paging::API) ? 0 :
                     ATTR_K   * !!(p & Paging::K)           |
                     ATTR_nX1 * ((nxs & nxu) | (xnx & nxu)) |
                     ATTR_nX0 * ((nxs ^ nxu) &  xnx)        |
                     ATTR_W   * !!(p & Paging::W)           |
                     ATTR_R   * !!(p & Paging::R)           |
                     a.share() << 8 | a.cache_s2() << 2 | !l * ATTR_nL | ATTR_A | ATTR_P;
        }

        auto page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                      !!(val & ATTR_K)                        * Paging::K  |
                                     !(!(val & ATTR_nX1) ^ !(val & ATTR_nX0)) * Paging::XS |
                                       !(val & ATTR_nX1)                      * Paging::XU |
                                      !!(val & ATTR_W)                        * Paging::W  |
                                      !!(val & ATTR_R)                        * Paging::R);
        }

        auto page_ma (unsigned) const
        {
            return Memattr { Memattr::Share (BIT_RANGE (1, 0) & val >> 8),
                             Memattr::Cache (!!(BIT_RANGE (5, 4) & val) * 4 + (BIT_RANGE (1, 0) & val >> 2)) };
        }

        Npt() = default;
        Npt (Entry e) : Pte (e) {}
};

class Nptp final : public Ptab<Npt, uint64_t, uint64_t>
{
    private:
        static uint64_t current CPULOCAL;

    public:
        explicit Nptp (OAddr v = 0) : Ptab (Npt (v)) {}

        ALWAYS_INLINE
        inline void make_current (Vmid vmid) const
        {
            auto const vttbr { static_cast<uint64_t>(vmid) << 48 | root_addr() };

            if (current != vttbr)
                asm volatile ("msr vttbr_el2, %x0; isb" : : "rZ" (current = vttbr) : "memory");
        }

        ALWAYS_INLINE
        inline void invalidate (Vmid vmid) const
        {
            make_current (vmid);

            asm volatile ("tlbi vmalls12e1is    ;"  // Invalidate TLB entries
                          "dsb  ish             ;"  // Ensure TLB invalidation completed
                          "isb                  ;"  // Ensure fetched instructions use new translation
                          : : : "memory");
        }

        static void init();
};

// Sanity checks
static_assert (Npt::lev() == 3);
static_assert (Npt::lev_bit (0) == 9 && Npt::lev_bit (1) == 9 && Npt::lev_bit (2) == 10);
static_assert (Npt::lev_idx (0, BITN (Npt::ibits) - 1) == 511 && Npt::lev_idx (1, BITN (Npt::ibits) - 1) == 511 && Npt::lev_idx (2, BITN (Npt::ibits) - 1) == 1023);
