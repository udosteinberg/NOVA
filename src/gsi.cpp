/*
 * Global System Interrupts (GSI)
 *
 * Copyright (C) 2006-2010, Udo Steinberg <udo@hypervisor.org>
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
#include "dmar.h"
#include "ec.h"
#include "gsi.h"
#include "hip.h"
#include "ioapic.h"
#include "keyb.h"
#include "lapic.h"
#include "regs.h"
#include "sm.h"
#include "stdio.h"

Gsi         Gsi::gsi_table[NUM_GSI];
unsigned    Gsi::irq_table[NUM_IRQ];
unsigned    Gsi::row;

void Gsi::setup()
{
    for (unsigned i = 0; i < NUM_IRQ; i++) {
        gsi_table[i].msk = 1;
        gsi_table[i].trg = 0;  // edge
        gsi_table[i].pol = 0;  // high
        gsi_table[i].vec = static_cast<uint8>(VEC_GSI + i);
        irq_table[i] = i;
    }

    for (unsigned i = NUM_IRQ; i < NUM_GSI; i++) {
        gsi_table[i].msk = 1;
        gsi_table[i].trg = 1;  // level
        gsi_table[i].pol = 1;  // low
        gsi_table[i].vec = static_cast<uint8>(VEC_GSI + i);
    }
}

void Gsi::init()
{
    if (!Cmdline::nospinner)
        row = screen.init_spinner (0);

    for (unsigned gsi = 0; gsi < NUM_GSI; gsi++) {
        Gsi::gsi_table[gsi].sm = new Sm (0, &Pd::kern, gsi);
        mask (gsi);
    }
}

uint64 Gsi::set (unsigned long gsi, unsigned long cpu, unsigned rid)
{
    assert (gsi < NUM_GSI);

    gsi_table[gsi].msk = 0;
    gsi_table[gsi].vec = static_cast<uint8>(VEC_GSI + gsi);

    uint32 msi_addr = 0, msi_data = 0;

    Ioapic *ioapic = gsi_table[gsi].ioapic;

    if (ioapic) {
        ioapic->set_cpu (gsi, Dmar::ire() ? 0 : cpu);
        ioapic->set_irt (gsi, gsi_table[gsi].msk << 16 | gsi_table[gsi].trg << 15 | gsi_table[gsi].pol << 13 | gsi_table[gsi].vec);
        rid = ioapic->get_rid();
    } else {
        msi_addr = 0xfee00000 | (Dmar::ire() ? 3ul << 3 : cpu << 12);
        msi_data = Dmar::ire() ? gsi : gsi_table[gsi].vec;
    }

    Dmar::set_irt (gsi, rid, cpu, VEC_GSI + gsi, gsi_table[gsi].trg);

    return static_cast<uint64>(msi_addr) << 32 | msi_data;
}

void Gsi::mask (unsigned long gsi)
{
    if (EXPECT_TRUE (gsi < NUM_GSI && gsi_table[gsi].ioapic)) {
        gsi_table[gsi].msk = 1;
        gsi_table[gsi].ioapic->set_irt (gsi, gsi_table[gsi].msk << 16 | gsi_table[gsi].trg << 15 | gsi_table[gsi].pol << 13 | gsi_table[gsi].vec);
    }

    if (EXPECT_FALSE (row))
        screen.put (row, 5 + gsi, Console_vga::COLOR_WHITE, 'M');
}

void Gsi::unmask (unsigned long gsi)
{
    if (EXPECT_TRUE (gsi < NUM_GSI && gsi_table[gsi].ioapic)) {
        gsi_table[gsi].msk = 0;
        gsi_table[gsi].ioapic->set_irt (gsi, gsi_table[gsi].msk << 16 | gsi_table[gsi].trg << 15 | gsi_table[gsi].pol << 13 | gsi_table[gsi].vec);
    }

    if (EXPECT_FALSE (row))
        screen.put (row, 5 + gsi, Console_vga::COLOR_WHITE, 'U');
}

void Gsi::vector (unsigned vector)
{
    unsigned gsi = vector - VEC_GSI;

    if (gsi == Keyb::gsi)
        Keyb::interrupt();

    else if (gsi == Acpi::gsi)
        Acpi::interrupt();

    else if (gsi_table[gsi].trg)
        mask (gsi);

    Lapic::eoi();

    gsi_table[gsi].sm->up();

    Counter::count (Counter::gsi[gsi], Console_vga::COLOR_LIGHT_YELLOW, 5 + vector - NUM_EXC);
}
