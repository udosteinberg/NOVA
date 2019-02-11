/*
 * DMA Page Table (DPT)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#include "pte.hpp"

class Dpt : public Pte<Dpt, uint64, 4, 9, true>
{
    public:
        static mword ord;

        enum
        {
            DPT_R   = 1UL << 0,
            DPT_W   = 1UL << 1,
            DPT_S   = 1UL << 7,

            PTE_P   = DPT_R | DPT_W,
            PTE_S   = DPT_S,
            PTE_N   = DPT_R | DPT_W,
        };
};
