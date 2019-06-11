/*
 * Virtual-Machine Identifier (VMID)
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

#include "macros.hpp"
#include "types.hpp"

class Vmid
{
    private:
        uint16 const val;

        static uint16 allocator;

        // FIXME: Handle overflow
        static inline auto alloc()
        {
            return __atomic_fetch_add (&allocator, 1, __ATOMIC_SEQ_CST);
        }

    public:
        inline Vmid() : val (alloc()) {}

        auto vmid() const { return val & BIT_RANGE (7, 0); }
};
