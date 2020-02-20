/*
 * Console: NS16550 UART
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

#include "cmdline.hpp"
#include "console_uart_ns16550.hpp"
#include "hpt.hpp"

INIT_PRIORITY (PRIO_CONSOLE) Console_ns16550 Console_ns16550::con;

Console_ns16550::Console_ns16550()
{
    if (!Cmdline::serial)
        return;

    char *mem = static_cast<char *>(Hpt::remap (0));
    if (!(base = *reinterpret_cast<uint16 *>(mem + 0x400)) &&
        !(base = *reinterpret_cast<uint16 *>(mem + 0x402)))
        return;

    init();

    enable();
}
