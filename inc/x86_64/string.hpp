/*
 * String Functions
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

#include "compiler.hpp"
#include "types.hpp"

extern "C" __attribute__((nonnull, used))
inline void *memcpy (void *d, void const *s, size_t n)
{
    auto dst { static_cast<char *>(d) };
    auto src { static_cast<char const *>(s) };

    while (n--)
        *dst++ = *src++;

    return d;
}

extern "C" __attribute__((nonnull, used))
inline void *memset (void *d, int c, size_t n)
{
    auto dst { static_cast<char *>(d) };

    while (n--)
        *dst++ = static_cast<char>(c);

    return d;
}

extern "C" __attribute__((nonnull))
inline int memcmp (void const *s1, void const *s2, size_t n)
{
    auto p1 { static_cast<unsigned char const *>(s1) };
    auto p2 { static_cast<unsigned char const *>(s2) };

    for (; n--; p1++, p2++)
        if (*p1 != *p2)
            return *p1 - *p2;

    return 0;
}

extern "C" __attribute__((nonnull))
inline int strcmp (char const *s1, char const *s2)
{
    while (*s1 && *s1 == *s2)
        s1++, s2++;

    // Comparison must be done using unsigned characters
    return static_cast<unsigned char>(*s1) - static_cast<unsigned char>(*s2);
}

extern "C" __attribute__((nonnull))
inline int strncmp (char const *s1, char const *s2, size_t n)
{
    if (!n)
        return 0;

    while (--n && *s1 && *s1 == *s2)
        s1++, s2++;

    // String comparison must be done using unsigned characters
    return static_cast<unsigned char>(*s1) - static_cast<unsigned char>(*s2);
}
