/*
 * Memory Type Range Registers (MTRR)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

class Mtrr : public List<Mtrr>
{
    private:
        uint64 const        base;
        uint64 const        mask;

        static unsigned     count;
        static unsigned     dtype;

        static Mtrr *       list;
        static Slab_cache   cache;

        uint64 size() const
        {
            return 1ULL << (static_cast<mword>(mask) ? bit_scan_forward (static_cast<mword>(mask >> 12)) + 12 :
                                                       bit_scan_forward (static_cast<mword>(mask >> 32)) + 32);
        }

    public:
        ALWAYS_INLINE
        explicit inline Mtrr (uint64 b, uint64 m) : List<Mtrr> (list), base (b), mask (m) {}

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        INIT
        static void init();

        INIT
        static unsigned memtype (uint64, uint64 &);
};
