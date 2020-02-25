/*
 * Paging Permissions
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

#include "macros.hpp"

class Paging final
{
    public:
        enum Permissions
        {
            NONE    = 0,                // No Permissions
            R       = BIT  (0),         // Read
            W       = BIT  (1),         // Write
            XU      = BIT  (2),         // Execute (User)
            XS      = BIT  (3),         // Execute (Supervisor)
            API     = XS | XU | W | R,
            U       = BIT (12),         // User Accessible
            K       = BIT (13),         // Kernel Memory
            G       = BIT (14),         // Global
            SS      = BIT (15),         // Shadow Stack
        };
};
