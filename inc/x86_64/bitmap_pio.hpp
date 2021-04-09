/*
 * PIO Bitmap
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

#include "bitmap.hpp"
#include "buddy.hpp"

class Bitmap_pio final
{
    public:
        static constexpr auto bits { BIT (16) };

        static bool valid (size_t s) { return s < bits; }

        void clr (size_t s)       {        pio.clr (s); }
        void set (size_t s)       {        pio.set (s); }
        bool tst (size_t s) const { return pio.tst (s); }

        /*
         * Allocate PIO bitmap
         *
         * @return      Pointer to the PIO bitmap (success) or nullptr (failure)
         */
        [[nodiscard]] static void *operator new (size_t) noexcept { return Buddy::alloc (1); }

        /*
         * Deallocate PIO bitmap
         *
         * @param ptr   Pointer to the PIO bitmap or nullptr
         */
        static void operator delete (void *ptr) { Buddy::free (ptr); }

    private:
        Bitmap<bits, true> pio;
};

// Sanity checks
static_assert (sizeof (Bitmap_pio) == PAGE_SIZE (0) << 1);
