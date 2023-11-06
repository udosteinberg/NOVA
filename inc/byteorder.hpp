/*
 * Byte Order (Endianness)
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

template <typename T, bool B>
class Byteorder
{
    private:
        T val;

        static constexpr auto bswap (uint128_t v) { return B ? __builtin_bswap128 (v) : v; }
        static constexpr auto bswap (uint64_t  v) { return B ? __builtin_bswap64  (v) : v; }
        static constexpr auto bswap (uint32_t  v) { return B ? __builtin_bswap32  (v) : v; }
        static constexpr auto bswap (uint16_t  v) { return B ? __builtin_bswap16  (v) : v; }
        static constexpr auto bswap (uint8_t   v) { return v; }

    public:
        operator T() const { return bswap (val); }

        explicit constexpr Byteorder (T v) : val { bswap (v) } {}

        void serialize (void *p) const { __builtin_memcpy (p, &val, sizeof (val)); }

        explicit Byteorder (void const *p) { __builtin_memcpy (&val, p, sizeof (val)); }
};

template <typename T> using Byteorder_be = Byteorder<T, __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__>;

using be_uint64_t = Byteorder_be<uint64_t>;
using be_uint32_t = Byteorder_be<uint32_t>;
using be_uint16_t = Byteorder_be<uint16_t>;
using be_uint8_t  = Byteorder_be<uint8_t>;

template <typename T> using Byteorder_le = Byteorder<T, __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__>;

using le_uint64_t = Byteorder_le<uint64_t>;
using le_uint32_t = Byteorder_le<uint32_t>;
using le_uint16_t = Byteorder_le<uint16_t>;
using le_uint8_t  = Byteorder_le<uint8_t>;
