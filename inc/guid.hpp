/*
 * Globally Unique Identifier (GUID)
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

#include "types.hpp"

struct Guid final
{
    uint32_t    part1;
    uint16_t    part2;
    uint16_t    part3;
    uint64_t    part4;

    bool operator== (Guid const &x) const
    {
        return x.part1 == part1 &&
               x.part2 == part2 &&
               x.part3 == part3 &&
               x.part4 == part4;
    }
};

static_assert (__is_standard_layout (Guid) && sizeof (Guid) == 16);
