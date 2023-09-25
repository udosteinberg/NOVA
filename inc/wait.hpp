/*
 * Completion Wait
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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
        static auto until (uint32_t ms, auto const &func)
        {
            for (uint64_t const t { Stc::ms_to_ticks (ms) }, b { Timer::time() }; !func(); pause())
                if (Timer::time() - b > t) [[unlikely]]
                    return false;

            return true;
        }
};
