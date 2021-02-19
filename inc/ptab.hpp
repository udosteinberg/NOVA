/*
 * Page Table
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

#include "atomic.hpp"
#include "bits.hpp"
#include "buddy.hpp"
#include "coherence.hpp"
#include "kmem.hpp"
#include "memattr.hpp"
#include "paging.hpp"
#include "status.hpp"
#include "util.hpp"

template<typename T, typename I, typename O> class Ptab
{
    public:
        using IAddr = I;
        using OAddr = O;

        class Entry
        {
            friend class Ptab;

            public:
                using OAddr = O;

                static constexpr unsigned bpl { bit_scan_msb (PAGE_SIZE (0) / sizeof (OAddr)) };

                static constexpr auto lev (unsigned b = T::ibits) { return (b - PAGE_BITS + bpl - 1) / bpl; }

                static constexpr auto lev_bit (unsigned)   { return bpl; }
                static constexpr auto lev_ent (unsigned l) { return BIT (T::lev_bit (l)); }
                static constexpr auto lev_ord (unsigned l = max_leaf_level) { return T::lev_bit (l) + l * bpl; }

                static constexpr auto lev_idx (unsigned l, IAddr i)
                {
                    // Determine shift count
                    auto const s { l * bpl + PAGE_BITS };

                    // C++ [expr.shift]: The behavior is undefined if the right operand is negative, or greater than or equal to the width of the promoted left operand
                    return s < type_bits<T>() ? (i >> s) % lev_ent (l) : 0;
                }

                static auto addr_mask() { return BIT64_RANGE (Memattr::obits - 1, PAGE_BITS); }

                static constexpr auto page_size (unsigned o) { return BITN (o + PAGE_BITS); }
                static constexpr auto offs_mask (unsigned o) { return page_size (o) - 1; }

                ALWAYS_INLINE inline auto atomic_member() { return &val; }

                ALWAYS_INLINE inline auto addr (unsigned l = 0) const { return val & addr_mask() & ~offs_mask (l * bpl); }

                auto table() const { return Kmem::phys_to_ptr<PTE> (addr()); }

                // Constructor
                constexpr Entry() = default;
                constexpr Entry (OAddr v) : val { v } {}

                bool operator== (Entry const &o) const { return val == o.val; }

            protected:
                enum class Type { PTAB, LEAF, HOLE };

                OAddr val;

            private:
                // Derived classes can shadow this constant with a variable
                static constexpr bool noncoherent { false };
        };

        Paging::Permissions lookup (IAddr, OAddr &, unsigned &, Memattr &) const;

        Status update (IAddr, OAddr, unsigned, Paging::Permissions, Memattr, bool &);

        Status update (IAddr v, OAddr p, unsigned o, Paging::Permissions pm, Memattr ma) { bool inv { false }; return update (v, p, o, pm, ma, inv); }

        [[nodiscard]] inline auto root_init (unsigned l = T::lev() - 1)
        {
            bool m { true }, inv { false };

            return walk (0, l, m, inv);
        }

        ALWAYS_INLINE
        inline auto root_addr() const
        {
            auto const val { root.load() };
            assert (val && !(val & ~Entry::addr_mask()));
            return val;
        }

        static void set_mll (unsigned l) { max_leaf_level = min (max_leaf_level, l); }

    protected:
        /*
         * CPU A (creating a new Page Table)            CPU B (walking Page Tables)
         *
         * (1) ST.RELAXED (init L0-Table)               (3) LD.ACQUIRE (L1-Table[slot])
         * (2) ST.RELEASE (L1-Table[slot] = L0-Table)   (4) LD.RELAXED (use L0-Table)
         *
         * Required Memory Ordering:
         * The stores that initialize a new Page Table must be observable
         * by remote CPUs before the new Page Table becomes observable.
         *
         * (1) happens before (2)                       (3) synchronizes with (2)
         *                                              (3) happens before (4)
         */
        using PTE = Atomic<Entry>;                      // ACQUIRE/RELEASE/ACQ_REL

        // Root of the page-table tree
        PTE root;

        // Constructor with an explicit root value
        explicit constexpr Ptab (T r) : root { r } {}

        /*
         * Factory for a flat array of n PTEs covering a page table at level l
         * Using a member function template delays type checking of class Ptab, which is not yet fully defined here (for gcc-12)
         */
        template<typename X = Ptab>
        [[nodiscard]] static PTE *create (unsigned l, OAddr p, size_t s) requires (std::standard_layout<X> && std::trivially_destructible<X>)
        {
            // Allocate at least one page, even if n requires less storage
            auto const o { max (T::lev_bit (l), T::bpl) - T::bpl };
            auto const n { T::lev_ent (l) };

            // Operator new is the implicit-object-creation function for the PTE[n] array
            auto const ptr { static_cast<PTE *>(operator new (sizeof (PTE) * n, o)) };
            if (!ptr) [[unlikely]]
                return nullptr;

            // Construct each PTE using global placement new
            for (unsigned i { 0 }; i < n; i++, p += s)
                ::new (ptr + i) PTE { p };

            // Ensure PTE observability
            Coherence::producer (T::noncoherent, ptr, sizeof (PTE) * n);

            return ptr;
        }

        [[nodiscard]] PTE *walk (IAddr, unsigned, bool &, bool &);

        /*
         * Deallocate entire page-table tree
         *
         * @param live  True if the tree may still be reachable by hardware, so its pages must be
         *              waitlisted; false if it was invalidated after being unlinked, so no TLB,
         *              IOTLB, or walk cache can reach it and its pages can be freed immediately
         */
        void deallocate_root (bool live) { replace (T::lev(), root, T { 0 }, live); }

        // Return the level at which x and y map to different slots of the same page table
        static constexpr unsigned diverge (IAddr x, IAddr y)
        {
            return (bit_scan_msb (x ^ y) - PAGE_BITS) / Entry::bpl;
        }

    private:
        // Maximum leaf level: 3 (512GB), 2 (1GB), 1 (2MB), 0 (4KB)
        static inline constinit unsigned max_leaf_level { 2 };

        // Deallocate page-table subtree at level l
        static void deallocate (PTE *ptr, unsigned l, bool live)
        {
            if (l)
                for (unsigned i { 0 }; i < T::lev_ent (l); i++)
                    replace (l, ptr[i], T { 0 }, live);

            // Waitlist pages after bootstrap when SMP/CPULOCAL is active
            operator delete (ptr, live && Cpu::online);
        }

        static bool replace (unsigned l, PTE &ptr, T pte, bool live)
        {
            T old;

            // Atomically replace old with new PTE
            ptr.exchange (old, pte);

            // If the old PTE referred to a PTAB, then deallocate it
            if (old.type (l) == Entry::Type::PTAB)
                deallocate (old.table(), l - 1, live);

            // Return if the change requires TLB invalidation
            return tlb_inv (old, pte, l);
        }

        /*
         * Determine whether a PTE change requires TLB invalidation
         *
         * @param o     Old PTE, as observed by the atomic that installed the new one
         * @param n     New PTE
         * @param l     Page-table level of the slot
         * @return      True if invalidation is required, false otherwise
         */
        static bool tlb_inv (T const &o, T const &n, unsigned l)
        {
            auto const ot { o.type (l) }, nt { n.type (l) };

            // PTAB => PTAB replacement never occurs
            assert (ot != Entry::Type::PTAB || nt != Entry::Type::PTAB);

            // HOLE => PTAB/LEAF requires invalidation only if not-present entries can be cached
            if (ot == Entry::Type::HOLE)
                return T::inv_notpresent && nt != Entry::Type::HOLE;

            // Type change requires invalidation, except if LEAF => PTAB splintering is permitted to produce multiple translation-identical entries
            if (ot != nt)
                return T::inv_splinter || nt != Entry::Type::PTAB;

            // LEAF => LEAF requires invalidation on address change, memory attribute change, R/W/XU/XS permission change/upgrade
            return o.addr (l) != n.addr (l) || o.page_ma (l) != n.page_ma (l) || (T::inv_upgrade ? o.page_pm() != n.page_pm() : o.page_pm() & ~n.page_pm());
        }

        [[nodiscard]] static void *operator new (size_t, unsigned o) noexcept
        {
            return Buddy::alloc (static_cast<Buddy::order_t>(o));
        }

        NONNULL
        static void operator delete (void *ptr, bool wait)
        {
            wait ? Buddy::wait (ptr) : Buddy::free (ptr);
        }
};
