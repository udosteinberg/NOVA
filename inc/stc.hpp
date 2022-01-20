/*
 * System Time Counter (STC)
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

class Stc
{
    public:
        /*
         * STC frequency in Hz
         *
         * - Must be uint64_t to express frequencies > 4.2 GHz
         * - Must be >= 1 KHz to convert 1 ms to non-zero ticks
         * - Defaults to 1 GHz until overridden by bootstrap code
         */
        static inline uint64_t freq { 1000'000'000 };

        /*
         * Convert relative system time to relative wall clock time
         *
         * @param ticks Relative system time in STC ticks
         * @return      Relative wall clock time in ms
         */
        static auto ticks_to_ms (uint64_t ticks)
        {
            // Will not overflow if ticks is at most 54 bits wide
            return ticks * 1000 / freq;
        }

        /*
         * Convert relative wall clock time to relative system time
         *
         * @param ms    Relative wall clock time in ms
         * @return      Relative system time in STC ticks
         */
        static auto ms_to_ticks (uint32_t ms)
        {
            // Will not overflow if ms is at most 32 (4.2 GHz), 31 (8.5 GHz), 30 (17.1 GHz) bits wide
            return freq * ms / 1000;
        }
};
