/*
 * Message Transfer Descriptor (MTD): Architecture-Specific Part (x86)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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
#include "mtd.hpp"

class Mtd_arch : public Mtd
{
    public:
        enum Item
        {
            POISON          = BIT  (0),

            GPR_0_7         = BIT  (1),
            GPR_8_15        = BIT  (2),
            RFLAGS          = BIT  (3),
            RIP             = BIT  (4),

            CTRL            = BIT  (5),
            QUAL            = BIT  (6),
            STA             = BIT  (7),
            INJ             = BIT  (8),

            CS_SS           = BIT  (9),
            DS_ES           = BIT (10),
            FS_GS           = BIT (11),
            TR              = BIT (12),
            LDTR            = BIT (13),
            GDTR            = BIT (14),
            IDTR            = BIT (15),

            PDPTE           = BIT (16),
            CR              = BIT (17),
            DR              = BIT (18),

            SYSENTER        = BIT (19),
            PAT             = BIT (20),
            EFER            = BIT (21),
            SYSCALL         = BIT (22),
            KERNEL_GS_BASE  = BIT (23),

            TLB             = BIT (30),
            FPU             = BIT (31),
        };

        inline Mtd_arch (uint32 v = 0) : Mtd (v) {}
};
