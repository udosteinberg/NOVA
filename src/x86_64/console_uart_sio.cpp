/*
 * Console: Super I/O (SIO) UART
 *
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

INIT_PRIORITY (PRIO_CONSOLE) Console_uart_sio Console_uart_sio::uart[] { Console_uart_sio { 0 }, Console_uart_sio { 1 } };

int Console_uart_sio::probe_isa (unsigned n)
{
    constexpr auto pci { Pci::pci (0, 1, 3) };

    if (Pci_arch::read (pci, Pci::Cfg::Reg32::DID_VID) == 0x71138086) {
        auto const reg { Pci_arch::read (pci, Pci::Cfg::Reg32 { 0x64 }) };
        if (reg & BIT (n * 4 + 27))
            return BIT_RANGE (2, 0) & reg >> (n * 4 + 24);
    }

    return -1;
}

int Console_uart_sio::probe_lpc (unsigned n)
{
    constexpr auto pci { Pci::pci (0, 31, 0) };

    if (static_cast<uint16_t>(Pci_arch::read (pci, Pci::Cfg::Reg32::DID_VID)) == 0x8086 && Pci_arch::read (pci, Pci::Cfg::Reg32::CCP_RID) >> 8 == 0x60100) {
        auto const reg { Pci_arch::read (pci, Pci::Cfg::Reg32 { 0x80 }) };
        if (reg & BIT (n + 16))
            return BIT_RANGE (2, 0) & reg >> (n * 4);
    }

    return -1;
}

Console_uart_sio::Regs Console_uart_sio::probe (unsigned n)
{
    Regs regs;

    int idx;
    if ((idx = probe_lpc (n)) != -1 || (idx = probe_isa (n)) != -1) [[likely]]
        if (!decode.test_and_set (BIT (idx))) [[likely]]
            regs.pio = pio[idx];

    return regs;
}
