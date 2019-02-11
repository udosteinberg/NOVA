/*
 * Bit Scan Functions
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#include "types.hpp"
#include "util.hpp"

ALWAYS_INLINE
inline long int bit_scan_reverse (mword val)
{
    if (EXPECT_FALSE (!val))
        return -1;

    asm volatile ("bsr %1, %0" : "=r" (val) : "rm" (val));

    return val;
}

ALWAYS_INLINE
inline long int bit_scan_forward (mword val)
{
    if (EXPECT_FALSE (!val))
        return -1;

    asm volatile ("bsf %1, %0" : "=r" (val) : "rm" (val));

    return val;
}

ALWAYS_INLINE
inline unsigned long max_order (mword base, size_t size)
{
    long int o = bit_scan_reverse (size);

    if (base)
        o = min (bit_scan_forward (base), o);

    return o;
}

ALWAYS_INLINE
static inline mword align_dn (mword val, mword align)
{
    val &= ~(align - 1);                // Expect power-of-2
    return val;
}

ALWAYS_INLINE
static inline mword align_up (mword val, mword align)
{
    val += (align - 1);                 // Expect power-of-2
    return align_dn (val, align);
}
