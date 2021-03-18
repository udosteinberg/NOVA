/*
 * High Precision Event Timer (HPET)
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

#include "list.hpp"
#include "slab.hpp"

class Hpet final : public List<Hpet>
{
    private:
        Paddr    const      phys;
        unsigned const      id;
        uint16              rid;

        static Hpet *       list;
        static Slab_cache   cache;

    public:
        ALWAYS_INLINE
        explicit inline Hpet (Paddr p, unsigned i) : List<Hpet> (list), phys (p), id (i), rid (0) {}

        ALWAYS_INLINE
        static inline bool claim_dev (unsigned r, unsigned i)
        {
            for (Hpet *hpet = list; hpet; hpet = hpet->next)
                if (hpet->rid == 0 && hpet->id == i) {
                    hpet->rid = static_cast<uint16>(r);
                    return true;
                }

            return false;
        }

        ALWAYS_INLINE
        static inline unsigned phys_to_rid (Paddr p)
        {
            for (Hpet *hpet = list; hpet; hpet = hpet->next)
                if (hpet->phys == p)
                    return hpet->rid;

            return ~0U;
        }

        [[nodiscard]] ALWAYS_INLINE
        static inline void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                cache.free (ptr);
        }
};
