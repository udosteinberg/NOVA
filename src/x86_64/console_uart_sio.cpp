/*
 * Console: Super I/O (SIO) UART
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

#include "console_uart_sio.hpp"
#include "pci.hpp"

INIT_PRIORITY (PRIO_CONSOLE) Console_uart_sio Console_uart_sio::uart0 { Pci::bdf (0, 31, 0) };

Console_uart_sio::Regs Console_uart_sio::probe (pci_t bdf)
{
    Regs regs;

    // Probe for Intel PCI-ISA Bridge
    if (Pci::cfg_read (bdf, Pci::Register16::VID) == 0x8086 && Pci::cfg_read (bdf, Pci::Register32::CCP_RID) >> 8 == 0x60100) {

        constexpr uint16_t port[8] { 0x3f8, 0x2f8, 0x220, 0x228, 0x238, 0x2e8, 0x338, 0x3e8 };

        auto const iod { Pci::cfg_read (bdf, Pci::Register32 { 0x80 }) };

        if (iod & BIT (16))
            regs.port = port[BIT_RANGE (2, 0) & iod];

        else if (iod & BIT (17))
            regs.port = port[BIT_RANGE (2, 0) & iod >> 4];
    }

    return regs;
}
