/*
 * Message Transfer Descriptor (MTD)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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
#include "types.hpp"

class Mtd
{
    public:
        mword val;

        enum Item
        {
            // IPC
            GPR_0_3         = 1UL << 0,
            GPR_4_7         = 1UL << 1,
            GPR_8_15        = 1UL << 2,
            RIP_LEN         = 1UL << 3,
            RFLAGS          = 1UL << 4,
            DS_ES           = 1UL << 5,
            FS_GS           = 1UL << 6,
            CS_SS           = 1UL << 7,
            TR              = 1UL << 8,
            LDTR            = 1UL << 9,
            GDTR            = 1UL << 10,
            IDTR            = 1UL << 11,
            CR              = 1UL << 12,
            DR              = 1UL << 13,
            SYSENTER        = 1UL << 14,
            QUAL            = 1UL << 15,
            CTRL            = 1UL << 16,
            INJ             = 1UL << 17,
            STA             = 1UL << 18,
            TSC             = 1UL << 19,
            EFER            = 1UL << 20,
            FPU             = 1UL << 31,
        };

        ALWAYS_INLINE
        inline explicit Mtd (mword v) : val (v) {}
};
