/*
 * Page Table Entry (x86)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "ptab.hpp"

/*
 * Generic Page Table Entry
 */
template<typename T, typename I, typename O> class Pte : public Ptab<T, I, O>::Entry
{
    using E = typename Ptab<T, I, O>::Entry;

    public:
        // Default TLB invalidation requirements for x86 CPU (derived classes use overrides for deviations)
        static constexpr bool inv_upgrade    { false };     // TLB self-heals for permission upgrades
        static constexpr bool inv_splinter   { false };     // TLB produces multiple translation-identical entries
        static constexpr bool inv_notpresent { false };     // TLB never caches not-present entries

        // PTE type at level l
        constexpr auto type (unsigned l) const
        {
            if (E::val & T::ATTR_P)
                return l && !(E::val & T::ATTR_S) ? E::Type::PTAB : E::Type::LEAF;

            // The root slot carries no attributes and is never DEAD: nonzero means PTAB
            return E::val ? (l == T::lev() ? E::Type::PTAB : E::Type::DEAD) : E::Type::HOLE;
        }
};

/*
 * Long-Mode Page Table Entry
 */
template<typename T, typename I, typename O> class Pte_lpt : public Pte<T, I, O>
{
    using E = typename Ptab<T, I, O>::Entry;

    protected:
        enum
        {
            ATTR_nX     = BITN (63),    // Not Executable
            ATTR_S      = BITN  (7),    // Superpage
            ATTR_D      = BITN  (6),    // Dirty
            ATTR_A      = BITN  (5),    // Accessed
            ATTR_U      = BITN  (2),    // User
            ATTR_W      = BITN  (1),    // Writable
            ATTR_P      = BITN  (0),    // Present
        };

        static_assert (!(E::DEAD_MARKER & ATTR_P));

        // Memory attributes: key bits + PAT index [2:0] scattered to bits PAT/PCD/PWT
        static auto attr_ma (unsigned l, Memattr a)
        {
            auto const c { a.cache_s1() };

            return a.key_encode() | (c & BIT (2)) << (l ? 10 : 5) | (c & BIT_RANGE (1, 0)) << 3;
        }

    public:
        // AMD requires the same paging mode for Pte_hst and Pte_gst_amd
        static constexpr unsigned ibits { 48 };

        // Memory attributes gathered from the bit positions attr_ma scatters them to
        auto page_ma (unsigned l) const
        {
            return Memattr { Memattr::Cache ((E::val >> (l ? 10 : 5) & BIT (2)) | (E::val >> 3 & BIT_RANGE (1, 0))), Memattr::key_decode (E::val) };
        }
};

/*
 * Extended Page Table Entry (Intel VMX/VT-d)
 */
template<typename T, typename I, typename O> class Pte_ept : public Pte<T, I, O>
{
    using E = typename Ptab<T, I, O>::Entry;

    protected:
        enum
        {
            ATTR_XU     = BITN (10),    // Executable (User)
            ATTR_D      = BITN  (9),    // Dirty
            ATTR_A      = BITN  (8),    // Accessed
            ATTR_S      = BITN  (7),    // Superpage
            ATTR_I      = BITN  (6),    // Ignore PAT
            ATTR_XS     = BITN  (2),    // Executable (Supervisor)
            ATTR_W      = BITN  (1),    // Writable
            ATTR_R      = BITN  (0),    // Readable
            ATTR_P      = ATTR_XU | ATTR_XS | ATTR_W | ATTR_R,
        };

        static_assert (!(E::DEAD_MARKER & ATTR_P));

        // Memory attributes: key bits + raw memory type scattered to bits 5:3
        static auto attr_ma (Memattr a) { return a.key_encode() | a.cache_s2() << 3; }

    public:
        // Memory attributes gathered from the bit positions attr_ma scatters them to
        auto page_ma (unsigned) const
        {
            return Memattr { Memattr::ept_to_ca (E::val >> 3 & BIT_RANGE (2, 0)), Memattr::key_decode (E::val) };
        }
};
