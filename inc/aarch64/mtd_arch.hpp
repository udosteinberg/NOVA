/*
 * Message Transfer Descriptor (MTD): Architecture-Specific Part (ARM)
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
            ICI             = BIT  (1),

            GPR             = BIT  (2),
            FPR             = BIT  (3),
            EL0_SP          = BIT  (4),
            EL0_IDR         = BIT  (5),

            A32_SPSR        = BIT  (7),
            A32_DACR_IFSR   = BIT  (8),

            EL1_SP          = BIT (10),
            EL1_IDR         = BIT (11),
            EL1_ELR_SPSR    = BIT (12),
            EL1_ESR_FAR     = BIT (13),
            EL1_AFSR        = BIT (14),
            EL1_TTBR        = BIT (15),
            EL1_TCR         = BIT (16),
            EL1_MAIR        = BIT (17),
            EL1_VBAR        = BIT (18),
            EL1_SCTLR       = BIT (19),
            EL1_MDSCR       = BIT (20),

            EL2_HCR         = BIT (23),
            EL2_IDR         = BIT (24),
            EL2_ELR_SPSR    = BIT (25),
            EL2_ESR_FAR     = BIT (26),
            EL2_HPFAR       = BIT (27),

            TMR             = BIT (30),
            GIC             = BIT (31),
        };

        inline Mtd_arch (uint32 v = 0) : Mtd (v) {}
};
