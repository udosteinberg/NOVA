/*
 * Low-Level Functions
 *
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "compiler.hpp"
#include "types.hpp"

ALWAYS_INLINE
inline uint64 div64 (uint64 n, uint32 d, uint32 *r)
{
    uint64 q = n / d;

    *r = static_cast<uint32>(n % d);

    return q;
}

ALWAYS_INLINE
static inline void pause()
{
    asm volatile ("yield" : : : "memory");
}

NORETURN ALWAYS_INLINE
inline void shutdown()
{
    for (;;)
        asm volatile ("wfi" : : : "memory");
}
