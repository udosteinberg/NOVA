/*
 * PIO Bitmap
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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
#include "buddy.hpp"
#include "compiler.hpp"

class Bitmap_pio final
{
    private:
        static constexpr auto sels { BITN (16) };
        static constexpr auto bits { 8 * sizeof (uintptr_t) };

        // I/O Bitmap
        struct {
            Atomic<uintptr_t> bmp { ~0UL };
        } io[sels / bits];

        static constexpr auto idx (unsigned long s) { return s % sels / bits; }
        static constexpr auto msk (unsigned long s) { return BITN (s % bits); }

        inline auto &sel_to_bmp (unsigned long s)       { return io[idx (s)].bmp; }
        inline auto &sel_to_bmp (unsigned long s) const { return io[idx (s)].bmp; }

    public:
        static inline bool sel_valid (unsigned long s) { return s < sels; }

        inline void clr (unsigned long s)       {        sel_to_bmp (s) &= ~msk (s); }
        inline void set (unsigned long s)       {        sel_to_bmp (s) |=  msk (s); }
        inline bool tst (unsigned long s) const { return sel_to_bmp (s) &   msk (s); }

        /*
         * Allocate PIO bitmap
         *
         * @return      Pointer to the PIO bitmap (allocation success) or nullptr (allocation failure)
         */
        [[nodiscard]] ALWAYS_INLINE static void *operator new (size_t) noexcept
        {
            static_assert (sizeof (Bitmap_pio) == PAGE_SIZE (0) << 1);
            return Buddy::alloc (1);
        }

        /*
         * Deallocate PIO bitmap
         *
         * @param ptr   Pointer to the PIO bitmap (or nullptr)
         */
        ALWAYS_INLINE
        static void operator delete (void *ptr)
        {
            Buddy::free (ptr);
        }
};
