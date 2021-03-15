/*
 * Low-Level Functions
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

#include "compiler.hpp"
#include "types.hpp"

NORETURN ALWAYS_INLINE
inline void shutdown()
{
    for (;;)
        asm volatile ("cli; hlt");
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
