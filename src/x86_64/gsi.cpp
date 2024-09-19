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
#include "gsi.hpp"
#include "ioapic.hpp"
#include "lapic.hpp"
#include "sm.hpp"
#include "smmu.hpp"

Gsi      Gsi::gsi_table[NUM_GSI];
unsigned Gsi::irq_table[NUM_IRQ];

void Gsi::setup()
{
    for (unsigned gsi { 0 }; gsi < NUM_GSI; gsi++) {

        Space_obj::insert_root (Gsi::gsi_table[gsi].sm = new Sm (&Pd::kern, NUM_CPU + gsi));

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

uint64 Gsi::set (unsigned gsi, cpu_t cpu)
{
    Atomic<uintptr_t> eoi;

    gsi_table[gsi].dst = static_cast<uint8_t>(Lapic::id[cpu]);

    auto const ioapic { gsi_table[gsi].ioapic };
    if (ioapic) {
        ioapic->rte_set_compat (eoi, gsi, false, gsi_table[gsi].trg, gsi_table[gsi].pol, gsi_table[gsi].dst, gsi_to_vec (gsi));
        return 0;
    }

    return static_cast<uint64>(0xfee00000 | gsi_table[gsi].dst << 12) << 32 | gsi_to_vec (gsi);
}

void Gsi::mask (unsigned gsi)
{
    Atomic<uintptr_t> eoi;

    auto const ioapic { gsi_table[gsi].ioapic };
    if (ioapic)
        ioapic->rte_set_compat (eoi, gsi, true, gsi_table[gsi].trg, gsi_table[gsi].pol, gsi_table[gsi].dst, gsi_to_vec (gsi));
}

void Gsi::unmask (unsigned gsi)
{
    Atomic<uintptr_t> eoi;

    auto const ioapic { gsi_table[gsi].ioapic };
    if (ioapic)
        ioapic->rte_set_compat (eoi, gsi, false, gsi_table[gsi].trg, gsi_table[gsi].pol, gsi_table[gsi].dst, gsi_to_vec (gsi));
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
        Smmu::all_interrupt();

    else if (v >= VEC_IPI)
        handle_ipi (v - VEC_IPI);

    else if (v >= VEC_LVT)
        handle_lvt (v - VEC_LVT);

    else if (v >= VEC_GSI)
        handle_gsi (v - VEC_GSI);

    Lapic::eoi();
}
