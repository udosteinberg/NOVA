/*
 * Low-Level Functions
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "compiler.hpp"
#include "types.hpp"

static inline void pause()
{
    __builtin_ia32_pause();
}

[[noreturn]] static inline void shutdown()
{
    for (;;)
        asm volatile ("cli; hlt");
}

template <typename T>
ALWAYS_INLINE
static inline void flush (T t)
{
    asm volatile ("clflush %0" : : "m" (*t) : "memory");
}

ALWAYS_INLINE NONNULL
inline void *flush (void *d, size_t n)
{
    for (char *p = static_cast<char *>(d); p < static_cast<char *>(d) + n; p += 32)
        flush (p);

    return d;
}

ALWAYS_INLINE
static inline auto rdtsc()
{
    return __builtin_ia32_rdtsc();
}
