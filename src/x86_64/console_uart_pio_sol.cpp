/*
 * Console: Serial-Over-LAN (SOL)
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

#include "console_uart_pio_sol.hpp"
#include "pci.hpp"

INIT_PRIORITY (PRIO_CONSOLE) Console_uart_pio_sol Console_uart_pio_sol::singleton;

uint16 Console_uart_pio_sol::probe()
{
    constexpr auto bdf { Pci::bdf (0, 22, 3) };

    // Probe for Intel 16550-Compatible Serial Controller
    if (Pci::cfg_read (bdf, Pci::Register16::VID) != 0x8086 || Pci::cfg_read (bdf, Pci::Register32::CCP_RID) >> 8 != 0x70002)
        return 0;

    auto const bar { Pci::cfg_read (bdf, Pci::Register32::BAR0) };

    // Probe for I/O resources
    if (bar == BIT (0))
        return 0;

    // Enable I/O space
    Pci::cfg_rd_wr (bdf, Pci::Register16::CMD, BIT (0));

    return bar & BIT_RANGE (15, 3);
}
