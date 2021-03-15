/*
 * Timer: Architecture-Specific (x86)
 *
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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

#include "lapic.hpp"
#include "stc.hpp"

class Timer : private Stc
{
    public:
        ALWAYS_INLINE
        static inline auto time()
        {
            return Lapic::time();
        }

        ALWAYS_INLINE
        static inline void set_dln (uint64 ticks)
        {
            Lapic::set_timer (ticks);
        }

        ALWAYS_INLINE
        static inline void stop() {}

        ALWAYS_INLINE
        static inline void set_time (uint64 t)
        {
            Msr::write (Msr::Register::IA32_TSC, t);

            Timeout::sync();
        }
};
