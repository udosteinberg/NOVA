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
#include "cache.hpp"
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

                static constexpr auto addr_mask() { return BIT64_RANGE (Memattr::obits - 1, PAGE_BITS); }

                static constexpr auto page_size (unsigned o) { return BITN (o + PAGE_BITS); }
                static constexpr auto offs_mask (unsigned o) { return page_size (o) - 1; }

                ALWAYS_INLINE inline auto addr (unsigned l = 0) const { return val & addr_mask() & ~offs_mask (l * bpl); }

                // Constructor
                constexpr Entry() = default;
                constexpr Entry (OAddr v) : val { v } {}

                auto operator->() const { return static_cast<Ptab *>(Kmem::phys_to_ptr (addr())); }
                bool operator== (Entry const &o) const { return val == o.val; }

            protected:
                enum class Type { PTAB, LEAF, HOLE };

                OAddr val;

            private:
                // Derived classes can shadow this constant with a variable
                static constexpr bool noncoherent { false };
        };

        Paging::Permissions lookup (IAddr, OAddr &, unsigned &, Memattr &) const;

        Status update (IAddr, OAddr, unsigned, Paging::Permissions, Memattr);

        [[nodiscard]] inline auto root_init (unsigned l = T::lev() - 1) { return walk (0, l, true); }

        ALWAYS_INLINE
        inline auto root_addr() const
        {
            auto const val { Entry { entry }.val };
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

        PTE entry;

        // Constructor for a single most-derived PTE
        explicit constexpr Ptab (T e) : entry { e } {}

        /*
         * Constructor for a PTAB containing n consecutive most-derived PTEs
         * Using a member function template delays type checking of class Ptab, which is not yet fully defined here (for gcc-12)
         */
        template<typename X = Ptab>
        explicit Ptab (unsigned n, OAddr p, size_t s) requires (std::standard_layout<X> && std::trivially_destructible<X>)
        {
            // Construct every PTE using global placement new
            for (unsigned i { 0 }; i < n; i++, p += s)
                ::new (this + i) Ptab { T { p } };

            // Ensure PTE observability
            T::noncoherent ? Cache::data_clean (this, n * sizeof (entry)) : T::publish();
        }

        [[nodiscard]] PTE *walk (IAddr, unsigned, bool);

        // Return the level at which x and y map to different slots of the same page table
        static constexpr unsigned diverge (IAddr x, IAddr y)
        {
            return (bit_scan_msb (x ^ y) - PAGE_BITS) / Entry::bpl;
        }

    private:
        // Maximum leaf level: 3 (512GB), 2 (1GB), 1 (2MB), 0 (4KB)
        static inline constinit unsigned max_leaf_level { 2 };

        void deallocate (unsigned);

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
