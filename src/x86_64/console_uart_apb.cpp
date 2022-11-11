/*
 * Console: Advanced Peripheral Bus (APB)
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

#include "console_uart_apb.hpp"

INIT_PRIORITY (PRIO_CONSOLE) Console_uart_apb Console_uart_apb::uart[] { Console_uart_apb { bdf0, did0 },
                                                                         Console_uart_apb { bdf1, did1 },
                                                                         Console_uart_apb { bdf2, did2 } };

Console_uart_apb::Regs Console_uart_apb::probe (pci_t const bdf[], uint16_t const did[])
{
    Regs regs;

    for (auto ptr { bdf }; *ptr; ptr++) {

        if (Pci::cfg_read (*ptr, Pci::Register16::VID) == 0x8086 && Pci::match_list (Pci::cfg_read (*ptr, Pci::Register16::DID), did)) {

            regs.phys = static_cast<uint64_t>(Pci::cfg_read (*ptr, Pci::Register32::BAR1)) << 32 | (Pci::cfg_read (*ptr, Pci::Register32::BAR0) & BIT_RANGE (31, 12));
            regs.size = 4;

            Pci::cfg_rd_wr (*ptr, Pci::Register16::CMD, !!regs.phys * BIT (1));

            break;
        }
    }

    return regs;
}
