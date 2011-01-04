/*
 * x86-Specific Functions
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
static inline void wbinvd()
{
    asm volatile ("wbinvd" : : : "memory");
}

ALWAYS_INLINE
static inline void pause()
{
    asm volatile ("pause" : : : "memory");
}

ALWAYS_INLINE
static inline uint64 rdtsc()
{
    mword h, l;
    asm volatile ("rdtsc" : "=a" (l), "=d" (h));
    return static_cast<uint64>(h) << 32 | l;
}

ALWAYS_INLINE
static inline mword get_cr0()
{
    mword cr0;
    asm volatile ("mov %%cr0, %0" : "=r" (cr0));
    return cr0;
}

ALWAYS_INLINE
static inline void set_cr0 (mword cr0)
{
    asm volatile ("mov %0, %%cr0" : : "r" (cr0));
}

ALWAYS_INLINE
static inline mword get_cr2()
{
    mword cr2;
    asm volatile ("mov %%cr2, %0" : "=r" (cr2));
    return cr2;
}

ALWAYS_INLINE
static inline void set_cr2 (mword cr2)
{
    asm volatile ("mov %0, %%cr2" : : "r" (cr2));
}

ALWAYS_INLINE
static inline mword get_cr4()
{
    mword cr4;
    asm volatile ("mov %%cr4, %0" : "=r" (cr4));
    return cr4;
}

ALWAYS_INLINE
static inline void set_cr4 (mword cr4)
{
    asm volatile ("mov %0, %%cr4" : : "r" (cr4));
}
