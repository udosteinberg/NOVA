/*
 * Advanced Programmable Interrupt Controller (APIC)
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

class Apic
{
    public:
        enum Delivery_mode
        {
            DLV_FIXED       = 0U << 8,
            DLV_LOWEST_PRIO = 1U << 8,
            DLV_SMI         = 2U << 8,
            DLV_REMOTE_READ = 3U << 8,
            DLV_NMI         = 4U << 8,
            DLV_INIT        = 5U << 8,
            DLV_SIPI        = 6U << 8,
            DLV_EXTINT      = 7U << 8
        };

        enum Mask
        {
            UNMASKED        = 0U << 16,
            MASKED          = 1U << 16      // 0x10000
        };
};
