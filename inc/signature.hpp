/*
 * Signature Functions
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

class Signature
{
    public:
        /*
         * Convert 4-character ASCII string to uint32_t (little-endian)
         *
         * @param x     String of length 4
         * @return      uint32_t representation of the string
         */
        static constexpr uint32_t value (char const (&x)[5])
        {
            return static_cast<uint32_t>(x[3]) << 24 | static_cast<uint32_t>(x[2]) << 16 |
                   static_cast<uint32_t>(x[1]) <<  8 | static_cast<uint32_t>(x[0]);
        }

        /*
         * Convert 8-character ASCII string to uint64_t (little-endian)
         *
         * @param x     String of length 8
         * @return      uint64_t representation of the string
         */
        static constexpr uint64_t value (char const (&x)[9])
        {
            return static_cast<uint64_t>(x[7]) << 56 | static_cast<uint64_t>(x[6]) << 48 |
                   static_cast<uint64_t>(x[5]) << 40 | static_cast<uint64_t>(x[4]) << 32 |
                   static_cast<uint64_t>(x[3]) << 24 | static_cast<uint64_t>(x[2]) << 16 |
                   static_cast<uint64_t>(x[1]) <<  8 | static_cast<uint64_t>(x[0]);
        }
};
