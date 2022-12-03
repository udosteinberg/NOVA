/*
 * Signature Functions
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

#include "types.hpp"

struct Signature
{
    /*
     * Convert 4-character ASCII string to uint32_t
     *
     * @param s     String of length >= 4
     * @return      uint32_t little-endian representation of string[0...3]
     */
    [[nodiscard]] static constexpr auto u32 (char const *s)
    {
        using T = uint32_t;

        return T { static_cast<uint8_t>(s[3]) } << 24 | T { static_cast<uint8_t>(s[2]) } << 16 |
               T { static_cast<uint8_t>(s[1]) } <<  8 | T { static_cast<uint8_t>(s[0]) };
    }

    /*
     * Convert 8-character ASCII string to uint64_t
     *
     * @param s     String of length >= 8
     * @return      uint64_t little-endian representation of string[0...7]
     */
    [[nodiscard]] static constexpr auto u64 (char const *s)
    {
        using T = uint64_t;

        return T { static_cast<uint8_t>(s[7]) } << 56 | T { static_cast<uint8_t>(s[6]) } << 48 |
               T { static_cast<uint8_t>(s[5]) } << 40 | T { static_cast<uint8_t>(s[4]) } << 32 |
               T { static_cast<uint8_t>(s[3]) } << 24 | T { static_cast<uint8_t>(s[2]) } << 16 |
               T { static_cast<uint8_t>(s[1]) } <<  8 | T { static_cast<uint8_t>(s[0]) };
    }
};
