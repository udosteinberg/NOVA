/*
 * Generic Timer
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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
    private:
        static uint32 freq CPULOCAL;

        static uint32 frequency()
        {
            return Lapic::freq_tsc;
        }

    public:
        ALWAYS_INLINE
        static inline uint64 time()
        {
            return __builtin_ia32_rdtsc();
        }

        ALWAYS_INLINE
        static inline void set_dln (uint64 val)
        {
            Lapic::set_timer (val);
        }

        ALWAYS_INLINE
        static inline void stop() {}

        ALWAYS_INLINE
        static inline uint64 ticks_to_ms (uint64 ticks)
        {
            return ticks * 1000 / freq;
        }

        ALWAYS_INLINE
        static inline uint64 ms_to_ticks (unsigned ms)
        {
            return static_cast<uint64>(ms) * freq / 1000;
        }

        static void interrupt();
        static void init();
};
