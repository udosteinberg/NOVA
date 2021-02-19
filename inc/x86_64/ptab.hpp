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
#include "coherence.hpp"
#include "kmem.hpp"
#include "memattr.hpp"
#include "paging.hpp"
#include "rcu.hpp"
#include "status.hpp"
#include "tlb.hpp"
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

                ALWAYS_INLINE constexpr auto atomic_member() { return &val; }

                ALWAYS_INLINE auto addr (unsigned l = 0) const { return val & addr_mask() & ~offs_mask (l * bpl); }
                ALWAYS_INLINE auto table() const { return Kmem::phys_to_ptr<PTE> (addr()); }

                // Constructor
                constexpr Entry() = default;
                constexpr Entry (OAddr v) : val { v } {}

                constexpr bool operator== (Entry const &o) const { return val == o.val; }

            protected:
                // This ordering produces the best code
                enum class Type { HOLE, LEAF, PTAB, DEAD };

                // Dead marker is any nonzero value that sets no present bits in any format
                static constexpr OAddr DEAD_MARKER { 0xDEAD0000 };

                OAddr val;

            private:
                // Derived classes can shadow this constant with a variable
                static constexpr bool noncoherent { false };
        };

        Paging::Permissions lookup (IAddr, OAddr &, unsigned &, Memattr &) const;

        Status update (IAddr, OAddr, unsigned, Paging::Permissions, Memattr, Tlb::Invalidate &);

        Status update (IAddr v, OAddr p, unsigned o, Paging::Permissions pm, Memattr ma) { auto inv { Tlb::Invalidate::SELF }; return update (v, p, o, pm, ma, inv); }

        [[nodiscard]] auto root_init (unsigned l = T::lev() - 1)
        {
            bool m { true }; auto inv { Tlb::Invalidate::SELF };

            return walk (0, l, m, inv);
        }

        ALWAYS_INLINE
        auto root_addr() const
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

        [[nodiscard]] PTE *walk (IAddr, unsigned, bool &, Tlb::Invalidate &);

        /*
         * Deallocate a linked PTAB subtree that is no longer in use by HW/SW
         *
         * @param ptr   Array of n slots at level l
         * @param l     PTAB level (defaults to T::lev() of the root slot)
         * @param n     Number of slots (defaults to 1 root slot)
         */
        __attribute__((nonnull)) static void deallocate (PTE const *ptr, unsigned l = T::lev(), unsigned n = 1)
        {
            for (unsigned i { 0 }; i < n; i++) {

                T const pte { ptr[i].load() };

                // DEAD only occurs in unlinked subtrees and those are not reachable from the root
                assert (pte.type (l) != Entry::Type::DEAD);

                // If the PTE refers to a PTAB, then deallocate it
                if (pte.type (l) == Entry::Type::PTAB) [[unlikely]] {

                    auto const tbl { pte.table() };

                    // Only descend to levels > 0, because no PTAB can exist at level 0
                    if (l > 1)
                        deallocate (tbl, l - 1, T::lev_ent (l - 1));

                    // Deallocate immediately
                    operator delete (tbl, false);
                }
            }
        }

        // Return the level at which x and y map to different slots of the same page table
        static constexpr unsigned diverge (IAddr x, IAddr y)
        {
            return (bit_scan_msb (x ^ y) - PAGE_BITS) / Entry::bpl;
        }

    private:
        // Maximum leaf level below the root slot: 3 (512GB), 2 (1GB), 1 (2MB), 0 (4KB)
        static inline constinit unsigned max_leaf_level { min (T::lev() - 1, 2U) };

        /*
         * Dismantle an unlinked PTAB subtree that may still be in use by HW/SW
         *
         * @param pte   PTE of the unlinked parent slot referring to a level-l PTAB
         * @param l     PTAB level
         */
        static void dismantle (T const pte, unsigned l)
        {
            // The replace (PTAB => *) that unlinked the subtree root ranks Tlb::Invalidate::ALL
            auto const tbl { pte.table() };

            // Only mark slots at levels > 0 as DEAD, because no PTAB can exist at level 0
            if (l > 0) {

                auto const n { T::lev_ent (l) };

                // Dismantling sweeps in the opposite direction of update
                for (unsigned i { n }; i--; ) {

                    T old, dead { Entry::DEAD_MARKER };

                    // Atomically mark PTE as DEAD
                    tbl[i].exchange (old, dead);

                    // DEAD replacement never occurs
                    assert (old.type (l) != Entry::Type::DEAD);

                    // If the PTE referred to a PTAB, then dismantle it
                    if (old.type (l) == Entry::Type::PTAB) [[unlikely]]
                        dismantle (old, l - 1);
                }

                // Ensure PTE observability
                Coherence::producer (T::noncoherent, tbl, sizeof (PTE) * n);
            }

            // Deallocate after RCU grace period once bootstrap is complete and SMP/CPULOCAL is active
            operator delete (tbl, Cpu::online);
        }

        static bool replace (unsigned l, PTE &ptr, T pte, Tlb::Invalidate &inv)
        {
            T old { ptr.load() };

            // Abandoned: A dead PTE must not be replaced and signals the entire PTAB is doomed
            if (old.type (l) == Entry::Type::DEAD) [[unlikely]]
                return false;

            // Abandoned: A failure to replace the PTE indicates a concurrent operation prevailed
            if (!ptr.compare_exchange (old, pte)) [[unlikely]]
                return true;

            // Changed: If the replaced PTE referred to a PTAB then dismantle the subtree
            if (old.type (l) == Entry::Type::PTAB) [[unlikely]]
                dismantle (old, l - 1);

            // Determine if the PTE change requires ALL invalidation
            if (tlb_inv_all (old, pte, l))
                inv = Tlb::Invalidate::ALL;

            return true;
        }

        /*
         * Determine whether a PTE change requires ALL invalidation
         *
         * @param o     Old PTE observed by the atomic update
         * @param n     New PTE installed by the atomic update
         * @param l     PTAB level of the atomic update
         * @return      True if ALL invalidation is required, false otherwise
         */
        static bool tlb_inv_all (T const &o, T const &n, unsigned l)
        {
            auto const ot { o.type (l) }, nt { n.type (l) };

            // PTAB => PTAB replacement never occurs
            assert (ot != Entry::Type::PTAB || nt != Entry::Type::PTAB);

            // DEAD is neither replaced nor installed here: only dismantle writes it, and that path does not classify
            assert (ot != Entry::Type::DEAD && nt != Entry::Type::DEAD);

            // HOLE => PTAB/LEAF requires ALL-invalidation only if not-present entries can be cached
            if (ot == Entry::Type::HOLE)
                return T::inv_notpresent && nt != Entry::Type::HOLE;

            // Type change requires ALL-invalidation, except if LEAF => PTAB splintering is permitted to produce multiple translation-identical entries
            if (ot != nt)
                return T::inv_splinter || nt != Entry::Type::PTAB;

            // LEAF => LEAF requires ALL-invalidation on address change, memory attribute change, R/W/XU/XS permission change/upgrade
            return o.addr (l) != n.addr (l) || o.page_ma (l) != n.page_ma (l) || (T::inv_upgrade ? o.page_pm() != n.page_pm() : o.page_pm() & ~n.page_pm());
        }

        [[nodiscard]] static void *operator new (size_t, unsigned o) noexcept
        {
            return Buddy::alloc (static_cast<Buddy::order_t>(o));
        }

        NONNULL
        static void operator delete (void *ptr, bool wait)
        {
            wait ? Rcu::submit (ptr) : Buddy::free (ptr);
        }
};
