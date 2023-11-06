/*
 * Endianness
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

#include "std.hpp"

/*
 * Aligned Big/Little-Endian Trivially Copyable Type T
 */
template<typename T, bool B> requires (std::trivially_copyable<T>) class Aligned
{
    private:
        T val;

        static constexpr T bswap (T v)
        {
            if constexpr (B) {
                     if constexpr  (sizeof (T) == 16) return __builtin_bswap128 (v);
                else if constexpr  (sizeof (T) ==  8) return __builtin_bswap64  (v);
                else if constexpr  (sizeof (T) ==  4) return __builtin_bswap32  (v);
                else if constexpr  (sizeof (T) ==  2) return __builtin_bswap16  (v);
                else static_assert (sizeof (T) ==  1, "no __builtin_bswap for this size");
            }

            return v;
        }

    public:
        constexpr Aligned() = default;

        constexpr Aligned (T v) : val { bswap (v) } {}

        constexpr operator T() const { return bswap (val); }

        // Read path for volatile-qualified MMIO registers
        operator T() const volatile { return bswap (val); }
};

template<typename T> using Aligned_be = Aligned<T, __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__>;
template<typename T> using Aligned_le = Aligned<T, __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__>;

/*
 * Unaligned Big/Little-Endian Trivially Copyable Type T
 */
template<typename T, bool B> requires (std::trivially_copyable<T>) class Unaligned
{
    private:
        unsigned char val[sizeof (T)];

    public:
        constexpr Unaligned() = default;

        constexpr Unaligned (T v) : Unaligned { __builtin_bit_cast (Unaligned, Aligned<T, B> { v }) } {}

        constexpr operator T() const { return __builtin_bit_cast (Aligned<T, B>, *this); }
};

template<typename T> using Unaligned_be = Unaligned<T, __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__>;
template<typename T> using Unaligned_le = Unaligned<T, __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__>;

/*
 * Sanity checks
 */
static_assert (sizeof (uint64_t) == sizeof (Aligned  <uint64_t, true>));
static_assert (sizeof (uint64_t) == sizeof (Unaligned<uint64_t, true>));
static_assert (std::trivially_constructible<Aligned  <uint64_t, true>>);
static_assert (std::trivially_constructible<Unaligned<uint64_t, true>>);
static_assert (std::trivially_copyable     <Aligned  <uint64_t, true>>);
static_assert (std::trivially_copyable     <Unaligned<uint64_t, true>>);
