/*
 * Keyboard
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

#include "acpi.h"
#include "counter.h"
#include "cmdline.h"
#include "gsi.h"
#include "keyb.h"

unsigned Keyb::gsi = ~0u;

/*
 * Keyboard Initialization
 */
void Keyb::init()
{
    if (!Cmdline::keyb)
        return;

    // Disable scan code translation
    send_ctrl (CMD_RD_CCB);
    unsigned ccb = read_keyb();
#if 0
    send_ctrl (CMD_WR_CCB);
    send_keyb (ccb & ~CCB_XLATE);

    // Set scan code set 1
    send_keyb (KEYB_SCAN_CODE);
    if (read_keyb() == KEYB_ACK) {
        send_keyb(1);
        read_keyb();
    }
#endif

    send_keyb (KEYB_READ_ID);
    unsigned id = read_keyb() == KEYB_ACK ? read_keyb() | read_keyb() << 8 : 0;

    gsi = Gsi::irq_to_gsi (irq);
    trace (TRACE_KEYB, "KEYB: GSI:%#x CCB:%#x ID:%#x", gsi, ccb, id);

    Gsi::set (gsi);
}

void Keyb::interrupt()
{
    unsigned sts;

    while ((sts = status()) & STS_OUTB) {

        unsigned inp = input();

        // Mouse data
        if (sts & STS_AUXB)
            continue;

        if (inp & 0x80)
            continue;

        switch (inp) {
            case 0x1:               // esc
                Acpi::reset();
                send_ctrl (CMD_RESET);
                Io::out<uint8>(0xcf9, 0x6);
                break;
            case 0x2e:              // c
                Counter::dump();
                break;
            case 0x3b ... 0x42:     // f1-f8
                screen.set_page (inp - 0x3b);
                break;
        }
    }
}
