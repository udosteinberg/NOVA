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
#include "pci_arch.hpp"

INIT_PRIORITY (PRIO_CONSOLE) Console_uart_sio Console_uart_sio::uart[] { Console_uart_sio { Pci::pci (0, 31, 0), BIT (16), 0 },
                                                                         Console_uart_sio { Pci::pci (0, 31, 0), BIT (17), 4 } };

Console_uart_sio::Regs Console_uart_sio::probe (pci_t pci, unsigned e, unsigned s)
{
    Regs regs;

    // Probe for Intel PCI-ISA Bridge
    if (static_cast<uint16_t>(Pci_arch::read (pci, Pci::Cfg::Reg32::DID_VID)) == 0x8086 && Pci_arch::read (pci, Pci::Cfg::Reg32::CCP_RID) >> 8 == 0x60100) {

        constexpr uint16_t port[8] { 0x3f8, 0x2f8, 0x220, 0x228, 0x238, 0x2e8, 0x338, 0x3e8 };

        auto const reg { Pci_arch::read (pci, Pci::Cfg::Reg32 { 0x80 }) };

        if (reg & e)
            regs.port = port[BIT_RANGE (2, 0) & reg >> s];
    }

    return regs;
}
