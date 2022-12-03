/*
 * Checksum Functions
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

struct Checksum
{
    /*
     * Compute additive checksum
     *
     * @param p     Pointer to region of n T-sized values
     * @param n     Number of T values to checksum
     * @return      Sum of T values, trimmed to T
     */
    template<std::unsigned_integral T> [[nodiscard]] static auto additive (void const *p, size_t n)
    {
        auto ptr { std::start_lifetime_as_array<T> (p, n) };

        T val { 0 };

        while (n--)
            val += *ptr++;

        return val;
    }
};
