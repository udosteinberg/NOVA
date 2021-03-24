/*
 * Universally Unique Identifier (UUID)
 *
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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

class Uuid final
{
    private:
        uint64_t const uuid[2];

    public:
        constexpr bool operator== (Uuid const &x) const
        {
            return uuid[0] == x.uuid[0] && uuid[1] == x.uuid[1];
        }

        constexpr Uuid (uint32_t l, uint16_t m, uint16_t h, uint8_t const (&n)[8]) : uuid { uint64_t { h } << 48 | uint64_t { m } << 32 | l, uint64_t { n[7] } << 56 | uint64_t { n[6] } << 48 | uint64_t { n[5] } << 40 | uint64_t { n[4] } << 32 | uint32_t { n[3] } << 24 | uint32_t { n[2] } << 16 | uint16_t { n[1] } << 8 | n[0] } {}
};

static_assert (__is_standard_layout (Uuid) && sizeof (Uuid) == 16);
