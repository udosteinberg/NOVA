/*
 * Generic Bitmap
 *
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

#include "atomic.hpp"
#include "bits.hpp"

/*
 * Bitmap with T Bits
 */
template<unsigned T> class Bitmap final
{
    private:
        static constexpr auto cnt { bits<uintptr_t>() };

        static constexpr auto idx (unsigned long n) { return n / cnt; }
        static constexpr auto msk (unsigned long n) { return BITN (n % cnt); }

        Atomic<uintptr_t> bitmap[aligned_up (cnt, T) / cnt] {};

    public:
        void clr (unsigned long n)       {        bitmap[idx (n)] &= ~msk (n); }
        void set (unsigned long n)       {        bitmap[idx (n)] |=  msk (n); }
        bool tst (unsigned long n) const { return bitmap[idx (n)] &   msk (n); }
};

// Sanity checks
static_assert (sizeof (Bitmap<1 + sizeof (uintptr_t) *  0>) == sizeof (uintptr_t) * 1);
static_assert (sizeof (Bitmap<1 + sizeof (uintptr_t) *  8>) == sizeof (uintptr_t) * 2);
static_assert (sizeof (Bitmap<1 + sizeof (uintptr_t) * 16>) == sizeof (uintptr_t) * 3);
