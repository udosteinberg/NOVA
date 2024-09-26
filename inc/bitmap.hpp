/*
 * Atomic Bitmap
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

#include "atomic.hpp"
#include "bits.hpp"

/*
 * Atomic Bitmap of B Bits initialized with I
 */
template<size_t B, bool I = false> class Bitmap final
{
    private:
        static constexpr auto cnt { type_bits<uintptr_t>() };
        static constexpr auto idx (size_t s) { return s / cnt; }
        static constexpr auto msk (size_t s) { return BITN (s % cnt); }

        struct { Atomic<uintptr_t> val { I ? ~uintptr_t { 0 } : 0 }; } bitmap[aligned_up (cnt, B) / cnt];

    public:
        ALWAYS_INLINE inline void clr (size_t s)       {        bitmap[idx (s)].val &= ~msk (s); }
        ALWAYS_INLINE inline void set (size_t s)       {        bitmap[idx (s)].val |=  msk (s); }
        ALWAYS_INLINE inline bool tst (size_t s) const { return bitmap[idx (s)].val &   msk (s); }

        ALWAYS_INLINE inline void cfg (size_t s, bool b) { b ? set (s) : clr (s); }
};

// Sanity checks
static_assert (sizeof (Bitmap<1 + sizeof (uintptr_t) *  0>) == sizeof (uintptr_t) * 1);
static_assert (sizeof (Bitmap<1 + sizeof (uintptr_t) *  8>) == sizeof (uintptr_t) * 2);
static_assert (sizeof (Bitmap<1 + sizeof (uintptr_t) * 16>) == sizeof (uintptr_t) * 3);
