/*
 * Completion Wait
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

#include "lowlevel.hpp"
#include "timer.hpp"

class Wait final
{
    public:
        // Timed wait for lambda function to return true
        static bool until (uint32_t ms, auto const &func)
        {
            for (uint64_t const t { Stc::ms_to_ticks (ms) }, b { Timer::time() }; !func(); pause())
                if (Timer::time() - b > t) [[unlikely]]
                    return false;

            return true;
        }

        // Timed wait for external store of completion value v to naturally-aligned doorbell d
        template<std::integral T>
        [[nodiscard]] static bool doorbell (uint32_t ms, T volatile &d, T v) requires (alignof (T) >= sizeof (T))
        {
            return until (ms, [&] { return d == v; });
        }
};
