/*
 * Virtual-Machine Identifier (VMID)
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

#include "atomic.hpp"
#include "macros.hpp"
#include "types.hpp"

class Vmid final
{
    private:
        using vmid_t = uint16_t;

        vmid_t const val;

        static inline constinit Atomic<vmid_t> allocator { 0 };

        // FIXME: Handle overflow
        static vmid_t alloc() { return allocator++; }

    public:
        explicit Vmid() : val { alloc() } {}

        operator vmid_t() const { return val & BIT_RANGE (7, 0); }
};
