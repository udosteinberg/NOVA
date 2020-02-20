/*
 * Generic Timer
 *
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#include "compiler.hpp"
#include "lapic.hpp"
#include "types.hpp"

class Timer
{
    public:
        ALWAYS_INLINE
        static inline uint64 time()
        {
            return __builtin_ia32_rdtsc();
        }

        ALWAYS_INLINE
        static inline void set_dln (uint64 ticks)
        {
            Lapic::set_timer (ticks);
        }

        ALWAYS_INLINE
        static inline void stop() {}

        ALWAYS_INLINE
        static inline auto ticks_to_ms (uint64 ticks)
        {
            return ticks / Lapic::freq_tsc;
        }

        ALWAYS_INLINE
        static inline auto ms_to_ticks (uint32 ms)
        {
            // Note: ms must be non-zero and Lapic::freq_tsc must be >= 1kHz
            return static_cast<uint64>(ms) * Lapic::freq_tsc;
        }

        static void interrupt();

        ALWAYS_INLINE
        static inline void init() {}
};
