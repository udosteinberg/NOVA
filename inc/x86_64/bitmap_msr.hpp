/*
 * MSR Bitmap
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

#include "atomic.hpp"
#include "buddy.hpp"
#include "compiler.hpp"

class Bitmap_msr final
{
    private:
        static constexpr auto sels { BITN (13) };
        static constexpr auto bits { 8 * sizeof (uintptr_t) };

        // MSR bitmap
        struct {
            Atomic<uintptr_t> bmp { ~0UL };
        } r_lo[sels / bits], r_hi[sels / bits], w_lo[sels / bits], w_hi[sels / bits];

        inline auto &sel_to_bmp_r (unsigned long s)       { return (s < sels ? r_lo : r_hi)[s % sels / bits].bmp; }
        inline auto &sel_to_bmp_r (unsigned long s) const { return (s < sels ? r_lo : r_hi)[s % sels / bits].bmp; }
        inline auto &sel_to_bmp_w (unsigned long s)       { return (s < sels ? w_lo : w_hi)[s % sels / bits].bmp; }
        inline auto &sel_to_bmp_w (unsigned long s) const { return (s < sels ? w_lo : w_hi)[s % sels / bits].bmp; }

        static inline auto sel_bmask (unsigned long s) { return BITN (s % bits); }

    public:
        static inline bool sel_valid (unsigned long s) { return s < sels || (s >= 0xc0000000 && s < 0xc0000000 + sels); }

        inline void clr_r (unsigned long s)       {        sel_to_bmp_r (s) &= ~sel_bmask (s); }
        inline void set_r (unsigned long s)       {        sel_to_bmp_r (s) |=  sel_bmask (s); }
        inline bool tst_r (unsigned long s) const { return sel_to_bmp_r (s) &   sel_bmask (s); }

        inline void clr_w (unsigned long s)       {        sel_to_bmp_w (s) &= ~sel_bmask (s); }
        inline void set_w (unsigned long s)       {        sel_to_bmp_w (s) |=  sel_bmask (s); }
        inline bool tst_w (unsigned long s) const { return sel_to_bmp_w (s) &   sel_bmask (s); }

        /*
         * Allocate MSR bitmap
         *
         * @return      Pointer to the MSR bitmap (allocation success) or nullptr (allocation failure)
         */
        [[nodiscard]] ALWAYS_INLINE
        static inline void *operator new (size_t) noexcept
        {
            static_assert (sizeof (Bitmap_msr) == PAGE_SIZE);
            return Buddy::alloc (0);
        }

        /*
         * Deallocate MSR bitmap
         *
         * @param ptr   Pointer to the MSR bitmap (or nullptr)
         */
        ALWAYS_INLINE
        static inline void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                Buddy::free (ptr);
        }
};
