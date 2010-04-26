/*
 * Serial Console
 *
 * Author: Udo Steinberg <udo@hypervisor.org>
 * TU Dresden, Operating Systems Group
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
#include "ptab.h"
#include "x86.h"

void Console_serial::init()
{
    if (!Cmdline::serial)
        return;

    char *mem = static_cast<char *>(Ptab::master()->remap (0));
    if (!(base = *reinterpret_cast<uint16 *>(mem + 0x400)) &&
        !(base = *reinterpret_cast<uint16 *>(mem + 0x402)))
        return;

    out (LCR, 0x80);
    out (DLR_LO, 1);
    out (DLR_HI, 0);
    out (LCR, 3);
    out (IER, 0);
    out (FCR, 7);
    out (MCR, 3);

    initialized = true;
}

void Console_serial::putc (int c)
{
    if (c == '\n')
        putc ('\r');

    while (EXPECT_FALSE (!(in (LSR) & 0x20)))
        pause();

    out (THR, c);
}
