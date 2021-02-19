/*
 * Page Table Entry (PTE)
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

#include "buddy.hpp"
#include "cache.hpp"
#include "kmem.hpp"
#include "memattr.hpp"
#include "paging.hpp"
#include "util.hpp"

template <typename PTT, unsigned L, unsigned B, typename I, typename O>
class Pte
{
    public:
        typedef I IAddr;
        typedef O OAddr;

        static constexpr unsigned lev = L;
        static constexpr unsigned bpl = B;

        // 1GB (3), 2MB (2), 4KB (1)
        static void set_lps (unsigned l) { lim = min (lim, l * bpl - 1); }

        static OAddr page_size (unsigned o) { return 1UL << (o + PAGE_BITS); }
        static OAddr page_mask (unsigned o) { return page_size (o) - 1; }

        OAddr init_root (bool nc, unsigned l = L - 1)
        {
            return Kmem::ptr_to_phys (walk (0, l, true, nc));
        }

        Paging::Permissions lookup (IAddr, OAddr &, unsigned &, Memattr::Cacheability &, Memattr::Shareability &) const;

        void update (IAddr, OAddr, unsigned, Paging::Permissions, Memattr::Cacheability, Memattr::Shareability, bool = false);

    protected:
        OAddr val;

        OAddr addr (unsigned l = 0) const { return val & PTT::ADDR_MASK & ~page_mask (l * B); }

        Pte() = default;

        Pte (OAddr p, OAddr s, bool nc)
        {
            for (unsigned i = 0; i < 1UL << B; i++, p += s)
                this[i].val = p;

            if (nc)
                Cache::dcache_clean (this, PAGE_SIZE);
        }

        PTT *walk (IAddr, unsigned, bool, bool = false);

    private:
        static unsigned lim;

        void deallocate (unsigned);

        NODISCARD
        static inline void *operator new (size_t) noexcept
        {
            return Buddy::alloc (0, Buddy::Fill::BITS0);
        }

        static inline void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                Buddy::free (ptr);
        }
};
