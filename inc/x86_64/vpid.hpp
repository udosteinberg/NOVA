/*
 * Virtual Processor Identifier (VPID)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "alloc_bitmap.hpp"

struct Invvpid final
{
    enum class Type : uintptr_t
    {
        ADR = 0,    // Individual address
        SGL = 1,    // Single context
        ALL = 2,    // All contexts
        SRG = 3,    // Single context retaining globals
    };

    static bool invalidate (Type t, uint16_t vpid, uint64_t addr = 0)
    {
        uint128_t const desc { uint128_t { addr } << 64 | vpid };

        bool ret;
        asm volatile ("invvpid %1, %2" : "=@cca" (ret) : "m" (desc), "r" (std::to_underlying (t)));
        return ret;
    }
};

struct Vpid final
{
    /*
     * VPID is always 16 bits wide
     * Conceptually per-CPU, but we use a global allocator
     */
    static inline constinit Alloc_bitmap<BIT (16)> allocator;
};
