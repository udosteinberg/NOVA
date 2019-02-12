/*
 * Console: PrimeCell UART (PL011)
 *
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "arch.hpp"
#include "console_uart_pl011.hpp"
#include "lowlevel.hpp"

INIT_PRIORITY (PRIO_CONSOLE) Console_pl011 Console_pl011::con;

Console_pl011::Console_pl011()
{
    init();

    enable();
}

void Console_pl011::init()
{
    write<uint16> (Register::CR,  0);
    write<uint16> (Register::LCR, BIT_RANGE (6, 4));
    write<uint16> (Register::CR,  BIT (8) | BIT (0));
}

bool Console_pl011::fini()
{
    while (EXPECT_FALSE (read<uint16> (Register::FR) & BIT (3)))
        pause();

    return true;
}

void Console_pl011::outc (char c)
{
    while (EXPECT_FALSE (read<uint16> (Register::FR) & BIT (5)))
        pause();

    write<uint8> (Register::DR, c);
}
