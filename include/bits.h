/*
 * Bit Scan Functions
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "compiler.h"
#include "types.h"
#include "util.h"

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
inline uint64 div64 (uint64 n, uint32 d, uint32 *r)
{
    uint64 q;
    asm volatile ("divl %5;"
                  "xchg %1, %2;"
                  "divl %5;"
                  "xchg %1, %3;"
                  : "=A" (q),   // quotient
                    "=r" (*r)   // remainder
                  : "a"  (static_cast<uint32>(n >> 32)),
                    "d"  (0),
                    "1"  (static_cast<uint32>(n)),
                    "rm" (d));
    return q;
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
