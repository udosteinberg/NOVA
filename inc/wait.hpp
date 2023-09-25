/*
 * Completion Wait
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

#include "lowlevel.hpp"
#include "timer.hpp"

class Wait final
{
    public:
        template <typename T>
        static bool until (uint32_t ms, T const &func)
        {
            for (uint64_t const t { Stc::ms_to_ticks (ms) }, b { Timer::time() }; !func(); pause())
                if (EXPECT_FALSE (Timer::time() - b > t))
                    return false;

            return true;
        }
};
