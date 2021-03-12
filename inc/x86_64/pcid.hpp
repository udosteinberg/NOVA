/*
 * Process Context Identifier (PCID)
 *
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

class Pcid
{
    private:
        uint16 const val;

        static inline Atomic<uint16> allocator { 0 };

        // FIXME: Handle overflow
        static inline auto alloc()
        {
            return allocator++;
        }

    public:
        inline Pcid() : val (alloc()) {}

        inline operator auto() const { return val & BIT_RANGE (11, 0); }
};
