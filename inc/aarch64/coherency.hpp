/*
 * Coherency Support
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

#include "barrier.hpp"
#include "cache.hpp"

class Coherency final
{
    public:
        static void observe (void const *ptr, size_t size, bool coherent)
        {
            // A DSB ISHST is sufficient if memory is coherent
            if (coherent) [[likely]]
                return Barrier::wsb (Barrier::Domain::ISH);

            // Use 32 as conservative default because per-CPU line size is not yet available during init
            Cache::data_clean (ptr, size, 32);
        }
};
