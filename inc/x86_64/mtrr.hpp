/*
 * Memory Type Range Registers (MTRR)
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

#include "bits.hpp"
#include "list.hpp"
#include "slab.hpp"

class Mtrr final : public List<Mtrr>
{
    private:
        uint64_t const      base;
        uint64_t const      mask;

        static unsigned     count;
        static unsigned     dtype;

        static Mtrr *       list;
        static Slab_cache   cache;

        uint64_t size() const
        {
            return 1ULL << (static_cast<uintptr_t>(mask) ? bit_scan_forward (static_cast<uintptr_t>(mask >> 12)) + 12 :
                                                           bit_scan_forward (static_cast<uintptr_t>(mask >> 32)) + 32);
        }

    public:
        explicit Mtrr (uint64_t b, uint64_t m) : List<Mtrr> (list), base (b), mask (m) {}

        static void init();

        static unsigned memtype (uint64_t, uint64_t &);

        [[nodiscard]] static void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }

        static void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                cache.free (ptr);
        }
};
