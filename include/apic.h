/*
 * Advanced Programmable Interrupt Controller (APIC)
 *
 * Copyright (C) 2006-2008, Udo Steinberg <udo@hypervisor.org>
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

class Apic
{
    public:
        enum Delivery_mode
        {
            DLV_FIXED       = 0u << 8,
            DLV_LOWEST_PRIO = 1u << 8,
            DLV_SMI         = 2u << 8,
            DLV_REMOTE_READ = 3u << 8,
            DLV_NMI         = 4u << 8,
            DLV_INIT        = 5u << 8,
            DLV_SIPI        = 6u << 8,
            DLV_EXTINT      = 7u << 8
        };

        enum Destination_mode
        {
            DST_PHYSICAL    = 0u << 11,
            DST_LOGICAL     = 1u << 11      // 0x800
        };

        enum Delivery_status
        {
            STS_IDLE        = 0u << 12,
            STS_PENDING     = 1u << 12      // 0x1000
        };

        enum Polarity
        {
            POL_HIGH        = 0u << 13,
            POL_LOW         = 1u << 13      // 0x2000
        };

        enum Trigger
        {
            TRG_EDGE        = 0u << 15,
            TRG_LEVEL       = 1u << 15      // 0x8000
        };

        enum Mask
        {
            UNMASKED        = 0u << 16,
            MASKED          = 1u << 16      // 0x10000
        };

        enum Mode
        {
            ONESHOT         = 0u << 17,
            PERIODIC        = 1u << 17      // 0x20000
        };

        enum Shorthand
        {
            SH_NONE         = 0u << 18,
            SH_SELF         = 1u << 18,
            SH_ALL_INC_SELF = 2u << 18,
            SH_ALL_EXC_SELF = 3u << 18
        };
};
