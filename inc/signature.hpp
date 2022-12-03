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
     * Convert 4-character ASCII string to uint32_t (little-endian)
     *
     * @param s     String of length >= 4
     * @return      uint32_t representation of the string
     */
    [[nodiscard]] static constexpr auto u32 (char const *s)
    {
        return uint32_t { static_cast<uint8_t>(s[3]) } << 24 | uint32_t { static_cast<uint8_t>(s[2]) } << 16 |
               uint32_t { static_cast<uint8_t>(s[1]) } <<  8 | uint32_t { static_cast<uint8_t>(s[0]) };
    }

    /*
     * Convert 8-character ASCII string to uint64_t (little-endian)
     *
     * @param s     String of length >= 8
     * @return      uint64_t representation of the string
     */
    [[nodiscard]] static constexpr auto u64 (char const *s)
    {
        return uint64_t { static_cast<uint8_t>(s[7]) } << 56 | uint64_t { static_cast<uint8_t>(s[6]) } << 48 |
               uint64_t { static_cast<uint8_t>(s[5]) } << 40 | uint64_t { static_cast<uint8_t>(s[4]) } << 32 |
               uint64_t { static_cast<uint8_t>(s[3]) } << 24 | uint64_t { static_cast<uint8_t>(s[2]) } << 16 |
               uint64_t { static_cast<uint8_t>(s[1]) } <<  8 | uint64_t { static_cast<uint8_t>(s[0]) };
    }
};
