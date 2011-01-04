/*
 * Descriptor
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

class Descriptor
{
    protected:
        enum Granularity
        {
            BYTES                   = 0u << 23,
            PAGES                   = 1u << 23,
        };

        enum Size
        {
            BIT_16                  = 0u << 22,
            BIT_32                  = 1u << 22
        };

        enum Type
        {
            SYS_LDT                 =  2u << 8,
            SYS_TASK_GATE           =  5u << 8,
            SYS_TSS                 =  9u << 8,
            SYS_CALL_GATE           = 12u << 8,
            SYS_INTR_GATE           = 14u << 8,
            SYS_TRAP_GATE           = 15u << 8,

            DATA_R                  = 16u << 8,
            DATA_RA                 = 17u << 8,
            DATA_RW                 = 18u << 8,
            DATA_RWA                = 19u << 8,
            DATA_DOWN_R             = 20u << 8,
            DATA_DOWN_RA            = 21u << 8,
            DATA_DOWN_RW            = 22u << 8,
            DATA_DOWN_RWA           = 23u << 8,

            CODE_X                  = 24u << 8,
            CODE_XA                 = 25u << 8,
            CODE_XR                 = 26u << 8,
            CODE_XRA                = 27u << 8,
            CODE_CONF_X             = 28u << 8,
            CODE_CONF_XA            = 29u << 8,
            CODE_CONF_XR            = 30u << 8,
            CODE_CONF_XRA           = 31u << 8
        };
};

#pragma pack(1)
class Pseudo_descriptor
{
    private:
        uint16  limit;
        mword   base;

    public:
        ALWAYS_INLINE
        inline Pseudo_descriptor (uint16 l, mword b) : limit (l), base (b) {}
};
#pragma pack()
