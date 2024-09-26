/*
 * Atomic Bitmap
 *
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

#include "assert.hpp"
#include "atomic.hpp"
#include "bits.hpp"

/*
 * Atomic Bitmap with B Bits, each initialized with I
 */
template<size_t B, bool I, int L = __ATOMIC_ACQUIRE, int S = __ATOMIC_RELEASE, int M = __ATOMIC_ACQ_REL> class Bitmap
{
    protected:
        // Constraints
        static_assert (B > 0, "Bitmap size must be non-zero");

        using bitmap_t = uintptr_t;

        static constexpr auto bp0 {  bitmap_t {} };             // Bit-Pattern 0
        static constexpr auto bp1 { ~bitmap_t {} };             // Bit-Pattern 1
        static constexpr auto cnt { type_bits<bitmap_t>() };

        static constexpr auto bit (size_t w, size_t b) { return w * cnt + b; }
        static constexpr auto idx (size_t s) { assert (s < B); return s / cnt; }
        static constexpr auto msk (size_t s) { return bitmap_t { 1 } << s % cnt; }

        struct { Atomic<bitmap_t, L, S, M> val { I ? bp1 : bp0 }; } bitmap[aligned_up (cnt, B) / cnt];

    public:
        ALWAYS_INLINE inline void clr (size_t s) { bitmap[idx (s)].val &= ~msk (s); }
        ALWAYS_INLINE inline void set (size_t s) { bitmap[idx (s)].val |=  msk (s); }

        ALWAYS_INLINE inline bool tac (size_t s) { return bitmap[idx (s)].val.test_and_clr (msk (s)); }
        ALWAYS_INLINE inline bool tas (size_t s) { return bitmap[idx (s)].val.test_and_set (msk (s)); }

        ALWAYS_INLINE inline bool tst (size_t s) const { return bitmap[idx (s)].val & msk (s); }

        ALWAYS_INLINE inline void cfg (size_t s, bool b) { b ? set (s) : clr (s); }

        ALWAYS_INLINE inline void clr_all() { for (auto &i : bitmap) i.val = bp0; }
        ALWAYS_INLINE inline void set_all() { for (auto &i : bitmap) i.val = bp1; }
};

// Sanity checks
static_assert (sizeof (Bitmap<sizeof (uintptr_t) * __CHAR_BIT__ * 0 + 1, false>) == sizeof (uintptr_t) * 1);
static_assert (sizeof (Bitmap<sizeof (uintptr_t) * __CHAR_BIT__ * 1 + 0, false>) == sizeof (uintptr_t) * 1);
static_assert (sizeof (Bitmap<sizeof (uintptr_t) * __CHAR_BIT__ * 1 + 1, false>) == sizeof (uintptr_t) * 2);
static_assert (sizeof (Bitmap<sizeof (uintptr_t) * __CHAR_BIT__ * 2 + 0, false>) == sizeof (uintptr_t) * 2);
static_assert (sizeof (Bitmap<sizeof (uintptr_t) * __CHAR_BIT__ * 2 + 1, false>) == sizeof (uintptr_t) * 3);
