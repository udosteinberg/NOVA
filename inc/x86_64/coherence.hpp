/*
 * Coherence Management
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

#include "cache.hpp"

struct Coherence final
{
    /*
     * Producer-Side Coherence Management (Cacheline)
     */
    static void producer (bool noncoherent, void const *ptr)
    {
        if (noncoherent) [[unlikely]]
            Cache::data_clean (ptr);
    }

    /*
     * Producer-Side Coherence Management (Region)
     */
    static void producer (bool noncoherent, void const *ptr, size_t size)
    {
        if (noncoherent) [[unlikely]]
            Cache::data_clean (ptr, size);
    }
};
