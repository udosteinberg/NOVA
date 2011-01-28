/*
 * Keyboard
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "acpi.h"
#include "counter.h"
#include "cmdline.h"
#include "gsi.h"
#include "keyb.h"

unsigned Keyb::gsi = ~0u;

void Keyb::init()
{
    if (!Cmdline::keyb)
        return;

    while (status() & STS_OUTB)
        output();

    gsi = Gsi::irq_to_gsi (irq);

    trace (TRACE_KEYB, "KEYB: GSI:%#x", gsi);

    Gsi::set (gsi);
}

void Keyb::interrupt()
{
    unsigned sts;

    while ((sts = status()) & STS_OUTB) {

        unsigned out = output();

        if (sts & STS_AUXB)
            continue;

        if (out & 0x80)
            continue;

        switch (out) {
            case 0x1:               // esc
                Acpi::reset();
                Io::out<uint8>(0xcf9, 0x6);
                break;
            case 0x2e:              // c
                Counter::dump();
                break;
            case 0x3b ... 0x42:     // f1-f8
                screen.set_page (out - 0x3b);
                break;
        }
    }
}
