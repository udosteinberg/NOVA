/*
 * Message Transfer Descriptor (MTD)
 *
 * Copyright (C) 2008-2009, Udo Steinberg <udo@hypervisor.org>
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

class Mtd
{
    private:
        mword val;

    public:
        enum Item
        {
            // IPC
            GPR_ACDB        = 1ul << 0,
            GPR_BSD         = 1ul << 1,
            RSP             = 1ul << 2,
            RIP_LEN         = 1ul << 3,
            RFLAGS          = 1ul << 4,
            DS_ES           = 1ul << 5,
            FS_GS           = 1ul << 6,
            CS_SS           = 1ul << 7,
            TR              = 1ul << 8,
            LDTR            = 1ul << 9,
            GDTR            = 1ul << 10,
            IDTR            = 1ul << 11,
            CR              = 1ul << 12,
            DR              = 1ul << 13,
            SYSENTER        = 1ul << 14,
            QUAL            = 1ul << 15,
            CTRL            = 1ul << 16,
            INJ             = 1ul << 17,
            STA             = 1ul << 18,
            TSC             = 1ul << 19,
        };

        ALWAYS_INLINE
        inline explicit Mtd() {}

        ALWAYS_INLINE
        inline explicit Mtd (mword v) : val (v) {}

        ALWAYS_INLINE
        inline bool xfer (Item item) const
        {
            return val & item;
        }

        ALWAYS_INLINE
        inline unsigned long ui() const
        {
            return val & 0x3ff;
        }

        ALWAYS_INLINE
        inline unsigned long ti() const
        {
            return val >> 23;
        }
};
