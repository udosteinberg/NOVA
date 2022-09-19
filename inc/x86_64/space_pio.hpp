/*
 * PIO Space
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

#include "bitmap_pio.hpp"
#include "kmem.hpp"
#include "paging.hpp"
#include "space.hpp"
#include "status.hpp"

class Space_pio : public Space
{
    friend class Pd;

    private:
        Bitmap_pio *const bmp;

        static Space_pio nova;

        Space_pio();

        inline Space_pio (Bitmap_pio *b) : bmp (b) {}

        inline ~Space_pio()
        {
            delete bmp;
        }

        [[nodiscard]] Paging::Permissions lookup (unsigned long) const;

        void update (unsigned long, Paging::Permissions);

    public:
        [[nodiscard]] Status delegate (Space_pio const *, unsigned long, unsigned long, unsigned, unsigned);

        [[nodiscard]] inline auto get_phys() const { return Kmem::ptr_to_phys (bmp); }

        static void user_access (uint64 base, size_t size, Paging::Permissions p)
        {
            for (unsigned i { 0 }; i < size; i++)       // FIXME: Optimize
                nova.update (base + i, p);
        }
};
