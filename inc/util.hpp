/*
 * Utility Functions
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

#include "std.hpp"

template<std::integral T, typename... ARGS> requires (std::integral<ARGS> && ...) constexpr auto min (T head, ARGS... tail)
{
    T v { head };

    // Binary left fold over the comma operator with the lambda immediately invoked for each t in the tail pack
    ([&v](auto const &t) { v = t < v ? t : v; }(tail), ...);

    return v;
}

template<std::integral T, typename... ARGS> requires (std::integral<ARGS> && ...) constexpr auto max (T head, ARGS... tail)
{
    T v { head };

    // Binary left fold over the comma operator with the lambda immediately invoked for each t in the tail pack
    ([&v](auto const &t) { v = t > v ? t : v; }(tail), ...);

    return v;
}

template<std::unsigned_integral T> static constexpr bool is_power_of_2 (T v) { return v && !(v & (v - 1)); }

template<typename T> constexpr auto gcd (T v1, T v2)
{
    while (v2) {
        T const r { v1 % v2 };
        v1  = v2;
        v2  = r;
    }

    return v1;
}

/*
 * Computes v / d * m
 *
 * v * m / d is precise, but the multiplication can potentially overflow
 * v / d * m is imprecise, but using the remainder restores the precision
 */
constexpr auto div_mul (uint64_t v, uint64_t d, uint32_t m)
{
    auto const q { v / d };     // Quotient
    auto const r { v % d };     // Remainder

    return q * m + r * m / d;
}

template<typename T> constexpr bool match_list (T const list[], T id)
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
template<typename T> constexpr T *expose (T *p) noexcept
{
    [[maybe_unused]] auto x { reinterpret_cast<uintptr_t>(p) };
    return p;
}

// Sanity checks
static_assert (min (3, 1, 4, 1, 5, 9, 2, 6, 5) == 1);
static_assert (max (3, 1, 4, 1, 5, 9, 2, 6, 5) == 9);
