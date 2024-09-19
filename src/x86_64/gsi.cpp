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
#include "vectors.hpp"

Gsi      Gsi::gsi_table[NUM_GSI];
unsigned Gsi::irq_table[NUM_IRQ];

void Gsi::setup()
{
    for (unsigned gsi { 0 }; gsi < NUM_GSI; gsi++) {

        Space_obj::insert_root (Gsi::gsi_table[gsi].sm = new Sm (&Pd::kern, NUM_CPU + gsi));

        gsi_table[gsi].vec = gsi_to_vec (gsi);

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

uint64 Gsi::set (unsigned gsi, cpu_t cpu, unsigned rid)
{
    uintptr_t msi_addr, msi_data;

    uint8_t cfg = gsi_table[gsi].trg * BIT (1) | gsi_table[gsi].pol * BIT (2);

    auto const irt { Smmu::Grp::lookup_irt (gsi) };

    (void) Smmu::assign_int (irt, gsi, cpu, gsi_to_vec (gsi), rid, cfg, msi_addr, msi_data);

    return static_cast<uint64>(msi_addr) << 32 | msi_data;
}

void Gsi::mask (unsigned gsi)
{
    Ioapic *ioapic = gsi_table[gsi].ioapic;

    if (ioapic)
        ioapic->set_cfg (gsi, gsi_to_vec (gsi), true, gsi_table[gsi].trg, gsi_table[gsi].pol);
}

void Gsi::unmask (unsigned gsi)
{
    Ioapic *ioapic = gsi_table[gsi].ioapic;

    if (ioapic)
        ioapic->set_cfg (gsi, gsi_to_vec (gsi), false, gsi_table[gsi].trg, gsi_table[gsi].pol);
}

void Gsi::vector (unsigned vector)
{
    unsigned gsi = vector - VEC_GSI;

    if (gsi == Acpi::gsi)
        Acpi::interrupt();

    else if (gsi_table[gsi].trg)
        mask (gsi);

    Lapic::eoi();

    gsi_table[gsi].sm->up();
}
