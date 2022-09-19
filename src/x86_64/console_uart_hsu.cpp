/*
 * Console: High-Speed UART (HSU)
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

#include "console_uart_hsu.hpp"
#include "pci.hpp"

INIT_PRIORITY (PRIO_CONSOLE) Console_uart_hsu Console_uart_hsu::singleton { 1'843'200 };

Console_uart_hsu::Regs Console_uart_hsu::probe()
{
    Regs regs;

    constexpr auto bdf { Pci::bdf (0, 26, 0) };

    // Probe for Intel 16550-Compatible Serial Controller
    if (Pci::cfg_read (bdf, Pci::Register16::VID) == 0x8086 && Pci::cfg_read (bdf, Pci::Register32::CCP_RID) >> 8 == 0x70002) {

        regs.phys = Pci::cfg_read (bdf, Pci::Register32::BAR1) & BIT_RANGE (31, 8);
        regs.port = Pci::cfg_read (bdf, Pci::Register32::BAR0) & BIT_RANGE (15, 3);

        Pci::cfg_rd_wr (bdf, Pci::Register16::CMD, !!regs.phys * BIT (1) | !!regs.port * BIT (0));
    }

    return regs;
}
