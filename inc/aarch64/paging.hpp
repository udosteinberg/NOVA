/*
 * Paging: ARM
 *
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#ifdef __ASSEMBLER__

#define ATTR_P      BIT64  (0)      // Present
#define ATTR_nL     BIT64  (1)      // Not Large
#define ATTR_CA(X)  ((X) << 2)      // Cacheability
#define ATTR_SH(X)  ((X) << 8)      // Shareability
#define ATTR_A      BIT64 (10)      // Accessed
#define ATTR_nG     BIT64 (11)      // Not Global

#else

class Paging
{
    public:
        enum Permissions
        {
            R   = BIT  (0),         // Read
            W   = BIT  (1),         // Write
            XU  = BIT  (2),         // Execute (User)
            XS  = BIT  (3),         // Execute (Supervisor)
            API = XS | XU | W | R,
            U   = BIT (12),         // User Accessible
            K   = BIT (13),         // Kernel Memory
            G   = BIT (14),         // Global
        };
};

#endif
