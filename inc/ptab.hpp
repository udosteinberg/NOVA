/*
 * Page Table
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "atomic.hpp"
#include "bits.hpp"
#include "buddy.hpp"
#include "cache.hpp"
#include "kmem.hpp"
#include "memattr.hpp"
#include "paging.hpp"
#include "status.hpp"
#include "util.hpp"

template <typename T, typename I, typename O>
class Ptab
{
    public:
        using IAddr = I;
        using OAddr = O;

        class Entry
        {
            friend class Ptab;

            public:
                using OAddr = O;

                static constexpr unsigned bpl { bit_scan_reverse (PAGE_SIZE (0) / sizeof (OAddr)) };

                static constexpr auto lev (unsigned b = T::ibits) { return (b - PAGE_BITS + bpl - 1) / bpl; }

                static constexpr auto lev_bit (unsigned)   { return bpl; }
                static constexpr auto lev_ent (unsigned l) { return BIT (T::lev_bit (l)); }
                static constexpr auto lev_ord (unsigned l = mll) { return T::lev_bit (l) + l * bpl; }
                static constexpr auto lev_idx (unsigned l, IAddr i) { return (i >> (l * bpl + PAGE_BITS)) % lev_ent (l); }

                static constexpr auto addr_mask() { return BIT64_RANGE (Memattr::obits - 1, PAGE_BITS); }

                static constexpr auto page_size (unsigned o) { return BITN (o + PAGE_BITS); }
                static constexpr auto offs_mask (unsigned o) { return page_size (o) - 1; }

                ALWAYS_INLINE inline auto addr (unsigned l = 0) const { return val & addr_mask() & ~offs_mask (l * bpl); }

                ALWAYS_INLINE inline auto operator->() const { return static_cast<Ptab *>(Kmem::phys_to_ptr (addr())); }
                ALWAYS_INLINE inline bool operator== (Entry const &x) const { return val == x.val; }

                ALWAYS_INLINE inline Entry() = default;
                ALWAYS_INLINE inline Entry (OAddr v) : val (v) {}

            protected:
                enum class Type { PTAB, LEAF, HOLE };

                OAddr val;

            private:
                static constexpr bool noncoherent { false };
        };

        Paging::Permissions lookup (IAddr, OAddr &, unsigned &, Memattr &) const;

        Status update (IAddr, OAddr, unsigned, Paging::Permissions, Memattr);

        [[nodiscard]] inline auto root_init (unsigned l = T::lev() - 1) { return walk (0, l, true); }

        ALWAYS_INLINE
        inline auto root_addr() const
        {
            auto val { static_cast<Entry>(entry).val };
            assert (val && !(val & ~Entry::addr_mask()));
            return val;
        }

        static void set_mll (unsigned l) { mll = min (mll, l); }

    protected:
        using PTE = Atomic<Entry>;

        PTE entry;

        Ptab (Entry e) : entry (e) {}

        [[nodiscard]] PTE *walk (IAddr, unsigned, bool);

    private:
        // Maximum leaf level: 3 (512GB), 2 (1GB), 1 (2MB), 0 (4KB)
        static inline unsigned mll { 2 };

        ALWAYS_INLINE
        inline Ptab (unsigned n, OAddr p, OAddr s)
        {
            for (unsigned i { 0 }; i < n; i++, p += s)
                this[i].entry = Entry (p);

            // Ensure PTE observability
            T::noncoherent ? Cache::data_clean (this, n * sizeof (entry)) : T::publish();
        }

        void deallocate (unsigned);

        [[nodiscard]] static inline void *operator new (size_t, unsigned o) noexcept
        {
            return Buddy::alloc (static_cast<uint8_t>(o));
        }

        NONNULL ALWAYS_INLINE
        static inline void operator delete (void *ptr, bool wait)
        {
            wait ? Buddy::wait (ptr) : Buddy::free (ptr);
        }
};
