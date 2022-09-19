/*
 * Console: High-Speed UART (HSU)
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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
#include "pci_arch.hpp"

INIT_PRIORITY (PRIO_CONSOLE) Console_uart_hsu Console_uart_hsu::uart[] { Console_uart_hsu { Pci::pci (0, 26, 0) },
                                                                         Console_uart_hsu { Pci::pci (0, 26, 1) },
                                                                         Console_uart_hsu { Pci::pci (0, 26, 2) } };

Console_uart_hsu::Regs Console_uart_hsu::probe (pci_t pci)
{
    Regs regs;

    // Probe for Intel 16550-Compatible Serial Controller
    if (static_cast<uint16_t>(Pci_arch::read (pci, Pci::Cfg::Reg32::DID_VID)) == 0x8086 && Pci_arch::read (pci, Pci::Cfg::Reg32::CCP_RID) >> 8 == 0x70002) {

        regs.phys = Pci_arch::read (pci, Pci::Cfg::Reg32::BAR_1) & BIT_RANGE (31, 8);
        regs.port = Pci_arch::read (pci, Pci::Cfg::Reg32::BAR_0) & BIT_RANGE (15, 3);

        Pci_arch::rd_wr (pci, Pci::Cfg::Reg16::CMD, !!regs.phys * BIT (1) | !!regs.port * BIT (0));
    }

    return regs;
}
