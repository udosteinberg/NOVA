/*
 * Memory Type Range Registers (MTRR)
 *
 * Copyright (C) 2007-2008, Udo Steinberg <udo@hypervisor.org>
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

#include "compiler.h"
#include "types.h"

class Mtrr
{
    public:
        enum Type
        {
            UC = 0,
            WC = 1,
            WT = 4,
            WP = 5,
            WB = 6
        };

        uint32 count;
        uint32 dtype;

        struct {
            uint64  base;
            uint64  mask;
        } mtrr[8];

        INIT
        void save (mword base, size_t size);

        INIT
        void load();

        INIT
        static Type memtype (mword);
};
