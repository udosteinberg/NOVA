/*
 * Global System Interrupts (GSI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "acpi.hpp"
#include "dmar.hpp"
#include "gsi.hpp"
#include "ioapic.hpp"
#include "lapic.hpp"
#include "sm.hpp"
#include "vectors.hpp"

Gsi         Gsi::gsi_table[NUM_GSI];
unsigned    Gsi::irq_table[NUM_IRQ];

void Gsi::setup()
{
    for (unsigned gsi = 0; gsi < NUM_GSI; gsi++) {

        Space_obj::insert_root (Gsi::gsi_table[gsi].sm = new Sm (&Pd::kern, NUM_CPU + gsi));

        gsi_table[gsi].vec = static_cast<uint8>(VEC_GSI + gsi);

        if (gsi < NUM_IRQ) {
            irq_table[gsi] = gsi;
            gsi_table[gsi].trg = 0;
            gsi_table[gsi].pol = 0;
        } else {
            gsi_table[gsi].trg = 1;
            gsi_table[gsi].pol = 1;
        }
    }
}

uint64 Gsi::set (unsigned gsi, unsigned cpu, unsigned rid)
{
    uint32 msi_addr = 0, msi_data = 0, aid = Cpu::apic_id[cpu];

    Ioapic *ioapic = gsi_table[gsi].ioapic;

    if (ioapic) {
        ioapic->set_cpu (gsi, Dmar::ire() ? 0 : aid);
        ioapic->set_irt (gsi, gsi_table[gsi].irt);
        rid = ioapic->get_rid();
    } else {
        msi_addr = 0xfee00000 | (Dmar::ire() ? 3U << 3 : aid << 12);
        msi_data = Dmar::ire() ? gsi : gsi_table[gsi].vec;
    }

    Dmar::set_irt (gsi, rid, aid, VEC_GSI + gsi, gsi_table[gsi].trg);

    return static_cast<uint64>(msi_addr) << 32 | msi_data;
}

void Gsi::mask (unsigned gsi)
{
    Ioapic *ioapic = gsi_table[gsi].ioapic;

    if (ioapic)
        ioapic->set_irt (gsi, 1U << 16 | gsi_table[gsi].irt);
}

void Gsi::unmask (unsigned gsi)
{
    Ioapic *ioapic = gsi_table[gsi].ioapic;

    if (ioapic)
        ioapic->set_irt (gsi, 0U << 16 | gsi_table[gsi].irt);
}

void Gsi::handle_ipi (unsigned n)
{
    assert (n < NUM_IPI);

    Counter::req[n].inc();

    switch (n) {
        case 0: Sc::rrq_handler(); break;
        case 1: Sc::rke_handler(); break;
    }
}

void Gsi::handle_lvt (unsigned n)
{
    assert (n < NUM_LVT);

    Counter::loc[n].inc();

    switch (n) {
        case 0: Lapic::handle_timer(); break;
        case 1: Lapic::handle_error(); break;
        case 2: Lapic::handle_perfm(); break;
        case 3: Lapic::handle_therm(); break;
        case 4: Lapic::handle_cmchk(); break;
    }
}

void Gsi::handle_gsi (unsigned n)
{
    assert (n < NUM_GSI);

    if (n == Acpi::gsi)
        Acpi::interrupt();

    else if (gsi_table[n].trg)
        mask (n);

    gsi_table[n].sm->up();
}

void Gsi::handler (unsigned v)
{
    if (v >= VEC_FLT)
        Dmar::interrupt();

    else if (v >= VEC_IPI)
        handle_ipi (v - VEC_IPI);

    else if (v >= VEC_LVT)
        handle_lvt (v - VEC_LVT);

    else if (v >= VEC_GSI)
        handle_gsi (v - VEC_GSI);

    Lapic::eoi();
}
