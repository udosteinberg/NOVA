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

#include "memory.hpp"
#include "types.hpp"

class Mtd
{
    protected:
        mword mtd { 0 };

        inline explicit Mtd (mword v) : mtd (v) {}

    public:
        inline mword val() const { return mtd; }
};

class Mtd_arch : public Mtd
{
    public:
        enum Item
        {
            POISON          = 1UL << 0,

            GPR             = 1UL << 1,
            FPR             = 1UL << 2,
            EL0_SP          = 1UL << 3,
            EL0_IDR         = 1UL << 4,

            A32_SPSR        = 1UL << 7,
            A32_DACR_IFSR   = 1UL << 8,

            EL1_SP          = 1UL << 10,
            EL1_IDR         = 1UL << 11,
            EL1_ELR_SPSR    = 1UL << 12,
            EL1_ESR_FAR     = 1UL << 13,
            EL1_AFSR        = 1UL << 14,
            EL1_TTBR        = 1UL << 15,
            EL1_TCR         = 1UL << 16,
            EL1_MAIR        = 1UL << 17,
            EL1_VBAR        = 1UL << 18,
            EL1_SCTLR       = 1UL << 19,

            EL2_IDR         = 1UL << 20,
            EL2_ELR_SPSR    = 1UL << 21,
            EL2_ESR_FAR     = 1UL << 22,
            EL2_HPFAR       = 1UL << 23,
            EL2_HCR         = 1UL << 24,

            TMR             = 1UL << 30,
            GIC             = 1UL << 31,
        };

        inline explicit Mtd_arch (mword v) : Mtd (v) {}
};

class Mtd_user : public Mtd
{
    public:
        static unsigned const items = PAGE_SIZE / sizeof (mword);

        inline unsigned count() const { return static_cast<unsigned>(mtd % items) + 1; }

        inline explicit Mtd_user (mword v) : Mtd (v) {}
};
