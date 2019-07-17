/*
 * Guest Page Table (Stage-2)
 *
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
#include "vmid.hpp"

class Pte_gst final : public Pte<Pte_gst, uint64_t, uint64_t>
{
    friend class Pte;

    private:
        enum
        {
            ATTR_K      = BITN (55),    // Kernel Memory
            ATTR_nX1    = BITN (54),    // Not Executable
            ATTR_nX0    = BITN (53),    // Not Executable
            ATTR_A      = BITN (10),    // Accessed
            ATTR_W      = BITN  (7),    // Writable
            ATTR_R      = BITN  (6),    // Readable
            ATTR_nL     = BITN  (1),    // Not Large
            ATTR_P      = BITN  (0),    // Present
        };

        static_assert (!(DEAD_MARKER & ATTR_P));

    public:
#ifdef SERVER
        static constexpr unsigned ibits { 48 };
#else
        static constexpr unsigned ibits { 40 };
#endif

        static inline constinit bool xnx { true };

        static constexpr auto lev (unsigned b = ibits) { return (b - 4 - PAGE_BITS + bpl - 1) / bpl; }
        static constexpr auto lev_bit (unsigned l) { return l < lev() - 1 ? bpl : ibits - PAGE_BITS - l * bpl; }

        // PTE attributes for PTAB at level l
        static constexpr auto attr_ptab (unsigned l)
        {
            assert (0 < l && l < lev());

            return ATTR_nL | ATTR_P;
        }

        // PTE attributes for LEAF at level l
        static auto attr_leaf (unsigned l, Paging::Permissions p, Memattr a)
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

        constexpr auto page_pm() const
        {
            return Paging::Permissions (!val ? 0 :
                                      !!(val & ATTR_K)                        * Paging::K  |
                                     !(!(val & ATTR_nX1) ^ !(val & ATTR_nX0)) * Paging::XS |
                                       !(val & ATTR_nX1)                      * Paging::XU |
                                      !!(val & ATTR_W)                        * Paging::W  |
                                      !!(val & ATTR_R)                        * Paging::R);
        }

        constexpr auto page_ma (unsigned) const
        {
            return Memattr { Memattr::Share (BIT_RANGE (1, 0) & val >> 8),
                             Memattr::Cache (!!(BIT_RANGE (5, 4) & val) * 4 + (BIT_RANGE (1, 0) & val >> 2)) };
        }

        explicit constexpr Pte_gst() = default;
        explicit constexpr Pte_gst (Entry e) : Pte { e } {}
};

class Ptab_gst final : public Ptab<Pte_gst, uint64_t, uint64_t>
{
    private:
        static uint64_t current CPULOCAL;

    public:
        explicit constexpr Ptab_gst() : Ptab { Pte_gst { 0 } } {}

        ~Ptab_gst() { deallocate (&root); }

        ALWAYS_INLINE
        void make_current (uint16_t vmid) const
        {
            auto const vttbr { uint64_t { vmid } << 48 | root_addr() };

            if (current != vttbr)
                asm volatile ("msr vttbr_el2, %x0; isb" : : "rZ" (current = vttbr) : "memory");
        }

        [[nodiscard]] bool invalidate_self (uint16_t vmid) const
        {
            make_current (vmid);

            asm volatile ("tlbi vmalls12e1      ;"  // Invalidate TLB entries for SELF
                          "dsb  nsh             ;"  // Ensure TLB invalidation completed for SELF
                          "isb                  ;"  // Ensure fetched instructions use new translation
                          : : : "memory");

            return true;
        }

        [[nodiscard]] bool invalidate_all (uint16_t vmid) const
        {
            make_current (vmid);

            asm volatile ("tlbi vmalls12e1is    ;"  // Invalidate TLB entries for ALL
                          "dsb  ish             ;"  // Ensure TLB invalidation completed for ALL
                          "isb                  ;"  // Ensure fetched instructions use new translation
                          : : : "memory");

            return true;
        }

        static void init();
};

// Sanity Checks
static_assert (Pte_gst::ibits != 32 || (Pte_gst::lev() == 2 && Pte_gst::lev_bit (0) == 9 && Pte_gst::lev_bit (1) == 11));
static_assert (Pte_gst::ibits != 36 || (Pte_gst::lev() == 3 && Pte_gst::lev_bit (0) == 9 && Pte_gst::lev_bit (1) == 9 && Pte_gst::lev_bit (2) == 6));
static_assert (Pte_gst::ibits != 40 || (Pte_gst::lev() == 3 && Pte_gst::lev_bit (0) == 9 && Pte_gst::lev_bit (1) == 9 && Pte_gst::lev_bit (2) == 10));
static_assert (Pte_gst::ibits != 42 || (Pte_gst::lev() == 3 && Pte_gst::lev_bit (0) == 9 && Pte_gst::lev_bit (1) == 9 && Pte_gst::lev_bit (2) == 12));
static_assert (Pte_gst::ibits != 44 || (Pte_gst::lev() == 4 && Pte_gst::lev_bit (0) == 9 && Pte_gst::lev_bit (1) == 9 && Pte_gst::lev_bit (2) == 9 && Pte_gst::lev_bit (3) == 5));
static_assert (Pte_gst::ibits != 48 || (Pte_gst::lev() == 4 && Pte_gst::lev_bit (0) == 9 && Pte_gst::lev_bit (1) == 9 && Pte_gst::lev_bit (2) == 9 && Pte_gst::lev_bit (3) == 9));
static_assert (Pte_gst::ibits != 52 || (Pte_gst::lev() == 4 && Pte_gst::lev_bit (0) == 9 && Pte_gst::lev_bit (1) == 9 && Pte_gst::lev_bit (2) == 9 && Pte_gst::lev_bit (3) == 13));

static_assert (Pte_gst::ibits != 32 || (Pte_gst::lev_idx (0, BITN (Pte_gst::ibits) - 1) == 511 && Pte_gst::lev_idx (1, BITN (Pte_gst::ibits) - 1) == 2047));
static_assert (Pte_gst::ibits != 36 || (Pte_gst::lev_idx (0, BITN (Pte_gst::ibits) - 1) == 511 && Pte_gst::lev_idx (1, BITN (Pte_gst::ibits) - 1) == 511 && Pte_gst::lev_idx (2, BITN (Pte_gst::ibits) - 1) == 63));
static_assert (Pte_gst::ibits != 40 || (Pte_gst::lev_idx (0, BITN (Pte_gst::ibits) - 1) == 511 && Pte_gst::lev_idx (1, BITN (Pte_gst::ibits) - 1) == 511 && Pte_gst::lev_idx (2, BITN (Pte_gst::ibits) - 1) == 1023));
static_assert (Pte_gst::ibits != 42 || (Pte_gst::lev_idx (0, BITN (Pte_gst::ibits) - 1) == 511 && Pte_gst::lev_idx (1, BITN (Pte_gst::ibits) - 1) == 511 && Pte_gst::lev_idx (2, BITN (Pte_gst::ibits) - 1) == 4095));
static_assert (Pte_gst::ibits != 44 || (Pte_gst::lev_idx (0, BITN (Pte_gst::ibits) - 1) == 511 && Pte_gst::lev_idx (1, BITN (Pte_gst::ibits) - 1) == 511 && Pte_gst::lev_idx (2, BITN (Pte_gst::ibits) - 1) == 511 && Pte_gst::lev_idx (3, BITN (Pte_gst::ibits) - 1) == 31));
static_assert (Pte_gst::ibits != 48 || (Pte_gst::lev_idx (0, BITN (Pte_gst::ibits) - 1) == 511 && Pte_gst::lev_idx (1, BITN (Pte_gst::ibits) - 1) == 511 && Pte_gst::lev_idx (2, BITN (Pte_gst::ibits) - 1) == 511 && Pte_gst::lev_idx (3, BITN (Pte_gst::ibits) - 1) == 511));
static_assert (Pte_gst::ibits != 52 || (Pte_gst::lev_idx (0, BITN (Pte_gst::ibits) - 1) == 511 && Pte_gst::lev_idx (1, BITN (Pte_gst::ibits) - 1) == 511 && Pte_gst::lev_idx (2, BITN (Pte_gst::ibits) - 1) == 511 && Pte_gst::lev_idx (3, BITN (Pte_gst::ibits) - 1) == 8191));
