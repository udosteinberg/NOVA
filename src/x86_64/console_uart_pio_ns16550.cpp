/*
 * Console: NS16550 UART
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "console_uart_pio_ns16550.hpp"

INIT_PRIORITY (PRIO_CONSOLE) Console_uart_pio_ns16550 Console_uart_pio_ns16550::singleton { 1843200 };

uint16 Console_uart_pio_ns16550::probe()
{
    constexpr uint32 pci { BIT (31) | 0 << 16 | 31 << 11 | 0 << 8 };

    Io::out<uint32>(0xcf8, pci);
    if (Io::in<uint16>(0xcfc) != 0x8086)
        return 0;

    Io::out<uint32>(0xcf8, pci | 0x8);
    if (Io::in<uint32>(0xcfc) >> 8 != 0x060100)
        return 0;

    Io::out<uint32>(0xcf8, pci | 0x80);
    auto val = Io::in<uint32>(0xcfc);

    constexpr uint16 port[8] { 0x3f8, 0x2f8, 0x220, 0x228, 0x238, 0x2e8, 0x338, 0x3e8 };

    if (val & BIT (16))
        return port[BIT_RANGE (2, 0) & val];

    if (val & BIT (17))
        return port[BIT_RANGE (2, 0) & val >> 4];

    return 0;
}
