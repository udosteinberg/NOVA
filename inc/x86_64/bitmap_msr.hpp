/*
 * MSR Bitmap
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

class Bitmap_msr final
{
    public:
        static constexpr auto bits { BIT (13) };

        static bool valid (size_t s) { return s < bits || (s >= 0xc0000000 && s < 0xc0000000 + bits); }

        void clr_r (size_t s)       {        (s < bits ? rlo : rhi).clr (s % bits); }
        void set_r (size_t s)       {        (s < bits ? rlo : rhi).set (s % bits); }
        bool tst_r (size_t s) const { return (s < bits ? rlo : rhi).tst (s % bits); }

        void clr_w (size_t s)       {        (s < bits ? wlo : whi).clr (s % bits); }
        void set_w (size_t s)       {        (s < bits ? wlo : whi).set (s % bits); }
        bool tst_w (size_t s) const { return (s < bits ? wlo : whi).tst (s % bits); }

        /*
         * Allocate MSR bitmap
         *
         * @return      Pointer to the MSR bitmap (success) or nullptr (failure)
         */
        [[nodiscard]] static void *operator new (size_t) noexcept { return Buddy::alloc (0); }

        /*
         * Deallocate MSR bitmap
         *
         * @param ptr   Pointer to the MSR bitmap or nullptr
         */
        static void operator delete (void *ptr) { Buddy::free (ptr); }

    private:
        Bitmap<bits, true> rlo, rhi, wlo, whi;
};

// Sanity checks
static_assert (sizeof (Bitmap_msr) == PAGE_SIZE (0));
