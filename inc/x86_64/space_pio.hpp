/*
 * PIO Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

        Space_pio (Bitmap_pio *b) : bmp { b } {}

        ~Space_pio() { delete bmp; }

        [[nodiscard]] Paging::Permissions lookup (size_t) const;

        void update (size_t, Paging::Permissions);

    public:
        static constexpr uint8_t sbw { bit_scan_msb (Bitmap_pio::bits) };
        static constexpr uint8_t mco { sbw };

        [[nodiscard]] Status delegate (Space_pio const *, size_t, size_t, unsigned, unsigned);

        [[nodiscard]] auto get_phys() const { return Kmem::ptr_to_phys (bmp); }

        static void access_ctrl (uint64_t base, size_t size, Paging::Permissions perm)
        {
            for (unsigned i { 0 }; i < size; i++)       // FIXME: Optimize
                nova.update (base + i, perm);
        }
};
