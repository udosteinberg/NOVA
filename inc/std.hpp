/*
 * C++ Standard Functions
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

namespace std
{
    /*
     * See https://en.cppreference.com/w/cpp/utility/launder
     */
    template <typename T>
    [[nodiscard]] constexpr auto launder (T *p) noexcept
    {
        return __builtin_launder (p);
    }

    /*
     * See https://en.cppreference.com/w/cpp/utility/to_underlying
     */
    template <typename T>
    [[nodiscard]] constexpr auto to_underlying (T t) noexcept
    {
        return static_cast<__underlying_type (T)>(t);
    }
}
