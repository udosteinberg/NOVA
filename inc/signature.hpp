/*
 * Signature Functions
 *
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

#include "types.hpp"

class Signature
{
    public:
        /*
         * Convert 4-character ASCII string to uint32_t (little-endian)
         *
         * @param s     String of length >= 4
         * @return      uint32_t representation of the string
         */
        static constexpr auto u32 (char const *s)
        {
            return static_cast<uint32_t>(s[3]) << 24 | static_cast<uint32_t>(s[2]) << 16 |
                   static_cast<uint32_t>(s[1]) <<  8 | static_cast<uint32_t>(s[0]);
        }

        /*
         * Convert 8-character ASCII string to uint64_t (little-endian)
         *
         * @param s     String of length >= 8
         * @return      uint64_t representation of the string
         */
        static constexpr auto u64 (char const *s)
        {
            return static_cast<uint64_t>(s[7]) << 56 | static_cast<uint64_t>(s[6]) << 48 |
                   static_cast<uint64_t>(s[5]) << 40 | static_cast<uint64_t>(s[4]) << 32 |
                   static_cast<uint64_t>(s[3]) << 24 | static_cast<uint64_t>(s[2]) << 16 |
                   static_cast<uint64_t>(s[1]) <<  8 | static_cast<uint64_t>(s[0]);
        }
};
