/*
 * Global System Interrupts (GSI)
 *
 * Copyright (C) 2006-2009, Udo Steinberg <udo@hypervisor.org>
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
#include "assert.h"
#include "cmdline.h"
#include "counter.h"
#include "ec.h"
#include "gsi.h"
#include "hip.h"
#include "ioapic.h"
#include "keyb.h"
#include "lapic.h"
#include "pic.h"
#include "regs.h"
#include "sm.h"
#include "stdio.h"

Gsi         Gsi::gsi_table[NUM_GSI];
unsigned    Gsi::irq_table[NUM_IRQ];
unsigned    Gsi::row;

void Gsi::setup()
{
    for (unsigned i = 0; i < NUM_IRQ; i++) {
        gsi_table[i].cpu = 0;
        gsi_table[i].irt |= MASKED | TRG_EDGE | POL_HIGH | (VEC_GSI + i);
        irq_table[i] = i;
    }

    for (unsigned i = NUM_IRQ; i < NUM_GSI; i++) {
        gsi_table[i].cpu = 0;
        gsi_table[i].irt |= MASKED | TRG_LEVEL | POL_LOW | (VEC_GSI + i);
    }
}

void Gsi::init()
{
    if (!Cmdline::nospinner)
        row = screen.init_spinner (0);

    for (unsigned i = 0; i < NUM_GSI; i++)
        Gsi::gsi_table[i].sm = new Sm (&Pd::kern, i, 0);

    if (Acpi::mode == Acpi::APIC) {
        for (unsigned gsi = 0; gsi < NUM_GSI; gsi++)
            if (gsi_table[gsi].ioapic) {
                gsi_table[gsi].ioapic->set_cpu (gsi_table[gsi].pin, gsi_table[gsi].cpu);
                gsi_table[gsi].ioapic->set_irt (gsi_table[gsi].pin, gsi_table[gsi].irt);
            }
    } else {
        Hip::remove (Hip::FEAT_GSI);
        unsigned elcr = 0;
        for (unsigned gsi = 0; gsi < NUM_IRQ; gsi++)
            if (gsi_table[gsi].irt & TRG_LEVEL)
                elcr |= 1u << gsi;
        trace (TRACE_INTERRUPT, "ELCR: %#x", elcr);
        Io::out (ELCR_SLV, static_cast<uint8>(elcr >> 8));
        Io::out (ELCR_MST, static_cast<uint8>(elcr));
    }
}

void Gsi::mask (unsigned long gsi)
{
    switch (Acpi::mode) {

        case Acpi::APIC:
            if (EXPECT_TRUE (gsi < NUM_GSI && gsi_table[gsi].ioapic)) {
                gsi_table[gsi].irt |= MASKED;
                gsi_table[gsi].ioapic->set_irt (gsi_table[gsi].pin, gsi_table[gsi].irt);
            }
            break;

        case Acpi::PIC:
            if (EXPECT_TRUE (gsi < NUM_IRQ))
                (gsi & 8 ? Pic::slave : Pic::master).pin_mask (gsi & 7);
            break;
    }

    if (EXPECT_FALSE (row))
        screen.put (row, 5 + gsi, Console_vga::COLOR_WHITE, 'M');

    trace (TRACE_INTERRUPT, "GSI: %#lx masked", gsi);
}

void Gsi::unmask (unsigned long gsi)
{
    switch (Acpi::mode) {

        case Acpi::APIC:
            if (EXPECT_TRUE (gsi < NUM_GSI && gsi_table[gsi].ioapic)) {
                gsi_table[gsi].irt &= ~MASKED;
                gsi_table[gsi].ioapic->set_irt (gsi_table[gsi].pin, gsi_table[gsi].irt);
            }
            break;

        case Acpi::PIC:
            if (EXPECT_TRUE (gsi < NUM_IRQ))
                (gsi & 8 ? Pic::slave : Pic::master).pin_unmask (gsi & 7);
            break;
    }

    if (EXPECT_FALSE (row))
        screen.put (row, 5 + gsi, Console_vga::COLOR_WHITE, 'U');

    trace (TRACE_INTERRUPT, "GSI: %#lx unmasked", gsi);
}

void Gsi::eoi (unsigned gsi)
{
    if (Acpi::mode == Acpi::APIC) {
        assert (gsi < NUM_GSI);
        assert (gsi_table[gsi].ioapic);
        Lapic::eoi();
    } else {
        assert (gsi < NUM_IRQ);
        if (gsi & 8)
            Pic::slave.eoi();
        Pic::master.eoi();
    }
}

void Gsi::vector (unsigned vector)
{
    unsigned gsi = vector - VEC_GSI;

    if (gsi == Keyb::gsi)
        Keyb::interrupt();

    else if (gsi == Acpi::gsi)
        Acpi::interrupt();

    else if (gsi_table[gsi].irt & TRG_LEVEL)
        mask (gsi);

    eoi (gsi);

    gsi_table[gsi].sm->up();

    Counter::count (Counter::gsi[gsi], Console_vga::COLOR_LIGHT_YELLOW, 5 + vector - NUM_EXC);
}
