/*
 * Interrupt Identifier
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

#include "types.hpp"

class Intid
{
    private:
        uint32_t val;

    public:
        constexpr auto seg() const { return static_cast<uint16_t>(val >> 16); }
        constexpr auto gsi() const { return static_cast<uint16_t>(val); }

        explicit constexpr Intid (uint32_t v) : val { v } {}
};
