/*
 * Utility Functions
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

template <typename T>
static constexpr T min (T v1, T v2)
{
    return v1 < v2 ? v1 : v2;
}

template <typename T>
static constexpr T max (T v1, T v2)
{
    return v1 > v2 ? v1 : v2;
}

template <typename T>
static constexpr T gcd (T v1, T v2)
{
    while (v2) {
        T r = v1 % v2;
        v1  = v2;
        v2  = r;
    }

    return v1;
}

template <typename T>
static constexpr bool match_list (T const list[], T id)
{
    for (auto ptr { list }; *ptr; ptr++)
        if (*ptr == id)
            return true;

    return false;
}

/*
 * Turns p into an "exposed" pointer. As a result, the standard allows
 * integers to be converted to pointers of the same provenance as p.
 */
template <typename T>
constexpr T *expose (T *p) noexcept
{
    [[maybe_unused]] auto x = reinterpret_cast<uintptr_t>(p);
    return p;
}
