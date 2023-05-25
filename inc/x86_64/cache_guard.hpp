/*
 * Cache Guard
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

#include "arch.hpp"
#include "cache.hpp"
#include "cr.hpp"

class Cache_guard final
{
    private:
        uintptr_t const cr0;

    public:
        Cache_guard() : cr0 { Cr::get_cr0() }
        {
            Cr::set_cr0 (cr0 | CR0_CD);     // No-fill cache mode
            Cache::data_clean();
        }

        ~Cache_guard()
        {
            Cache::data_clean();
            Cr::set_cr0 (cr0);              // Normal cache mode
        }

        Cache_guard            (Cache_guard const &) = delete;
        Cache_guard& operator= (Cache_guard const &) = delete;
};
