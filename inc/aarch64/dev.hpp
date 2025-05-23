/*
 * Device
 *
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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

#include "kmem.hpp"
#include "list.hpp"
#include "slab.hpp"

class Dev final : public List<Dev>
{
    private:
        unsigned const its;         // ID: ITS
        unsigned const dev;         // ID: Device
        void *   const tbl;         // Interrupt Translation Table

        static Slab_cache cache;                        // Device Slab Cache
        static inline constinit Dev *list { nullptr };  // Device List

    public:
        explicit Dev (unsigned i, unsigned d, void *t) : List { list }, its { i }, dev { d }, tbl { t } {}

        auto get_itt() const { return Kmem::ptr_to_phys (tbl); }

        static Dev *find (unsigned i, unsigned d)
        {
            for (auto l { list }; l; l = l->next)
                if (l->its == i && l->dev == d)
                    return l;

            return nullptr;
        }

        [[nodiscard]] static void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }

        static void operator delete (void *ptr)
        {
            cache.free (ptr);
        }
};
