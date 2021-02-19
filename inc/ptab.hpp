/*
 * Page Table
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

#include "atomic.hpp"
#include "bits.hpp"
#include "buddy.hpp"
#include "cache.hpp"
#include "kmem.hpp"
#include "memattr.hpp"
#include "paging.hpp"
#include "status.hpp"
#include "util.hpp"

template <typename T, typename I, typename O, unsigned L, unsigned M, bool C>
class Pagetable
{
    public:
        typedef I IAddr;
        typedef O OAddr;

        static constexpr auto lev = L;
        static constexpr auto bpl = bit_scan_reverse (PAGE_SIZE / sizeof (OAddr));

        class Entry
        {
            friend class Pagetable;

            public:
                typedef O OAddr;

                static constexpr auto lev = L;
                static constexpr auto page_size (unsigned o) { return BITN (o + PAGE_BITS); }
                static constexpr auto offs_mask (unsigned o) { return page_size (o) - 1; }

                ALWAYS_INLINE inline auto addr (unsigned l = 0) const { return val & T::ADDR_MASK & ~offs_mask (l * bpl); }
                ALWAYS_INLINE inline bool is_empty() const { return !val; }

                ALWAYS_INLINE inline auto operator->() const { return static_cast<Table *>(Kmem::phys_to_ptr (addr())); }
                ALWAYS_INLINE inline bool operator== (Entry const &x) const { return val == x.val; }

                ALWAYS_INLINE inline Entry() = default;
                ALWAYS_INLINE inline Entry (OAddr v) : val (v) {}

            protected:
                OAddr val;
        };

        Paging::Permissions lookup (IAddr, OAddr &, unsigned &, Memattr::Cacheability &, Memattr::Shareability &) const;

        Status update (IAddr, OAddr, unsigned, Paging::Permissions, Memattr::Cacheability, Memattr::Shareability, bool = false);

        NODISCARD
        inline auto root_init (bool nc, unsigned l = L - 1) { return walk (0, l, true, nc); }

        ALWAYS_INLINE
        inline auto root_addr() const
        {
            auto val = static_cast<Entry>(root).val;
            assert (val && !(val & ~T::ADDR_MASK));
            return val;
        }

        // Maximum leaf page size: 4 (512GB), 3 (1GB), 2 (2MB), 1 (4KB)
        static inline void set_leaf_max (unsigned l) { lim = min (lim, l * bpl); }

    protected:
        typedef Atomic<Entry> PTE;

        PTE root;

        inline Pagetable (Entry r) : root (r) {}

        NODISCARD
        PTE *walk (IAddr, unsigned, bool, bool = false);

    private:
        struct Table
        {
            static constexpr auto entries = BIT (bpl);

            PTE slot[entries];

            ALWAYS_INLINE
            inline Table (OAddr p, OAddr s, bool nc)
            {
                for (unsigned i = 0; i < entries; i++, p += s)
                    slot[i] = Entry (p);

                if (C && nc)
                    Cache::data_clean (this, PAGE_SIZE);
            }

            void deallocate (unsigned);

            NODISCARD ALWAYS_INLINE
            static inline void *operator new (size_t) noexcept
            {
                static_assert (sizeof (Table) == PAGE_SIZE);
                return Buddy::alloc (0);
            }

            NONNULL ALWAYS_INLINE
            static inline void operator delete (void *ptr, bool wait)
            {
                wait ? Buddy::wait (ptr) : Buddy::free (ptr);
            }
        };

        static inline unsigned lim { M * bpl };
};
