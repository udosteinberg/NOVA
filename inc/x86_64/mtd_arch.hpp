/*
 * Message Transfer Descriptor (MTD): Architecture-Specific Part (x86)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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

class Mtd_arch final : public Mtd
{
    public:
        enum Item
        {
            POISON          = BIT  (0),

            GPR_0_7         = BIT  (1),
            GPR_8_15        = BIT  (2),
            RFLAGS          = BIT  (3),
            RIP             = BIT  (4),

            STA             = BIT  (5),
            QUAL            = BIT  (6),
            CTRL            = BIT  (7),
            TPR             = BIT  (8),
            INJ             = BIT  (9),

            CS_SS           = BIT (10),
            DS_ES           = BIT (11),
            FS_GS           = BIT (12),
            TR              = BIT (13),
            LDTR            = BIT (14),
            GDTR            = BIT (15),
            IDTR            = BIT (16),

            PDPTE           = BIT (17),
            CR              = BIT (18),
            DR              = BIT (19),
            XSAVE           = BIT (20),
            SYSCALL         = BIT (21),
            SYSENTER        = BIT (22),
            PAT             = BIT (23),
            EFER            = BIT (24),
            KERNEL_GS_BASE  = BIT (25),
            TSC             = BIT (26),

            TLB             = BIT (29),
            FPU             = BIT (30),

            SPACES          = BIT (31),
        };

        explicit Mtd_arch (uint32 v = 0) : Mtd (v) {}
};
