/*
 * Universally Unique Identifier (UUID)
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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

struct Uuid final
{
    uint64_t const uuid[2];

    bool operator== (Uuid const &x) const
    {
        return uuid[0] == x.uuid[0] && uuid[1] == x.uuid[1];
    }

    bool operator!= (Uuid const &x) const
    {
        return !(*this == x);
    }
};

static_assert (__is_standard_layout (Uuid) && sizeof (Uuid) == 16);
