/*
 * Console: Super I/O (SIO) UART
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

        auto const reg { Pci_arch::read (pci, Pci::Cfg::Reg32 { 0x80 }) };
        auto const idx { reg >> s & BIT_RANGE (2, 0) };

        // Work around broken firmware that decodes ComA and ComB to the same port
        static constinit Atomic<uint8_t> decode { 0 };
        if (reg & e && !decode.test_and_set (BIT (idx)))
            regs.pio = pio[idx];
    }

    return regs;
}
