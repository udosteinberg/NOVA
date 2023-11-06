/*
 * Byte Order (Endianness)
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

#include "types.hpp"

/*
 * Aligned Big/Little-Endian Integral Type T
 */
template<typename T, bool B> class Aligned
{
    private:
        T val { 0 };

        static constexpr auto bswap (uint128_t v) { return B ? __builtin_bswap128 (v) : v; }
        static constexpr auto bswap (uint64_t  v) { return B ? __builtin_bswap64  (v) : v; }
        static constexpr auto bswap (uint32_t  v) { return B ? __builtin_bswap32  (v) : v; }
        static constexpr auto bswap (uint16_t  v) { return B ? __builtin_bswap16  (v) : v; }
        static constexpr auto bswap (uint8_t   v) { return v; }

    public:
        constexpr Aligned() = default;

        explicit constexpr Aligned (T v) : val { bswap (v) } {}

        operator T() const { return bswap (val); }
};

template<typename T> using Aligned_be = Aligned<T, __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__>;
template<typename T> using Aligned_le = Aligned<T, __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__>;

/*
 * Unaligned Big/Little-Endian Integral Type T
 */
template<typename T, bool B> class Unaligned
{
    private:
        char val[sizeof (T)] { 0 };

    public:
        constexpr Unaligned() = default;

        Unaligned (T v) { Aligned<T, B> tmp { v }; __builtin_memcpy (val, &tmp, sizeof (T)); }

        operator T() const { Aligned<T, B> tmp; __builtin_memcpy (&tmp, val, sizeof (T)); return tmp; }
};

template<typename T> using Unaligned_be = Unaligned<T, __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__>;
template<typename T> using Unaligned_le = Unaligned<T, __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__>;
