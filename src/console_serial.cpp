/*
 * Serial Console
 *
 * Copyright (C) 2005-2008, Udo Steinberg <udo@hypervisor.org>
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

#include "cmdline.h"
#include "console_serial.h"
#include "cpu.h"
#include "ptab.h"

void Console_serial::init()
{
    if (Cmdline::noserial)
        return;

    char *mem = static_cast<char *>(Ptab::master()->remap (0));
    if (!(base = *reinterpret_cast<uint16 *>(mem + 0x400)) &&
        !(base = *reinterpret_cast<uint16 *>(mem + 0x402)))
        return;

    out (LCR, LCR_DLAB);
    out (DLR_LOW,  1);
    out (DLR_HIGH, 0);
    out (LCR, LCR_DATA_BITS_8 | LCR_STOP_BITS_1);
    out (IER, 0);
    out (FCR, FCR_FIFO_ENABLE | FCR_RECV_FIFO_RESET | FCR_TMIT_FIFO_RESET);
    out (MCR, MCR_DTR | MCR_RTS);

    initialized = true;
}

void Console_serial::putc (int c)
{
    if (c == '\n')
        putc ('\r');

    while (EXPECT_FALSE (!(in (LSR) & LSR_TMIT_HOLD_EMPTY)))
        Cpu::pause();

    out (THR, c);
}
