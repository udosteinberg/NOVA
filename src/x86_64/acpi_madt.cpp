/*
 * Advanced Configuration and Power Interface (ACPI)
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
#include "acpi_madt.hpp"
#include "cpu.hpp"
#include "gsi.hpp"
#include "ioapic.hpp"
#include "pic.hpp"

void Acpi_table_madt::parse() const
{
    parse_entry (Acpi_apic::LAPIC,  &parse_lapic);
    parse_entry (Acpi_apic::IOAPIC, &parse_ioapic);
    parse_entry (Acpi_apic::INTR,   &parse_intr);

    if (flags & 1)
        Pic::init();
}

void Acpi_table_madt::parse_entry (Acpi_apic::Type type, void (*handler)(Acpi_apic const *)) const
{
    for (Acpi_apic const *ptr = apic; ptr < reinterpret_cast<Acpi_apic *>(reinterpret_cast<mword>(this) + length); ptr = reinterpret_cast<Acpi_apic *>(reinterpret_cast<mword>(ptr) + ptr->length))
        if (ptr->type == type)
            (*handler)(ptr);
}

void Acpi_table_madt::parse_lapic (Acpi_apic const *ptr)
{
    Acpi_lapic const *p = static_cast<Acpi_lapic const *>(ptr);

    if (p->flags & 1 && Cpu::online < NUM_CPU) {
        Cpu::acpi_id[Cpu::online]   = p->acpi_id;
        Cpu::apic_id[Cpu::online++] = p->apic_id;
    }
}

void Acpi_table_madt::parse_ioapic (Acpi_apic const *ptr)
{
    Acpi_ioapic const *p = static_cast<Acpi_ioapic const *>(ptr);

    Ioapic *ioapic = new Ioapic (p->phys, p->id, p->gsi);

    unsigned gsi = p->gsi;
    unsigned max = ioapic->irt_max();

    for (unsigned short i = 0; i <= max && gsi < NUM_GSI; i++, gsi++)
        Gsi::gsi_table[gsi].ioapic = ioapic;
}

void Acpi_table_madt::parse_intr (Acpi_apic const *ptr)
{
    Acpi_intr const *p = static_cast<Acpi_intr const *>(ptr);

    unsigned irq = p->irq;
    unsigned gsi = p->gsi;

    if (EXPECT_FALSE (gsi >= NUM_GSI || irq >= NUM_IRQ || p->bus))
        return;

    Gsi::irq_table[irq] = gsi;

    Gsi::gsi_table[gsi].pol = p->flags.pol == Acpi_inti::POL_LOW   || (p->flags.pol == Acpi_inti::POL_CONFORMING && irq == Acpi::irq);
    Gsi::gsi_table[gsi].trg = p->flags.trg == Acpi_inti::TRG_LEVEL || (p->flags.trg == Acpi_inti::TRG_CONFORMING && irq == Acpi::irq);

    if (irq == Acpi::irq)
        sci_overridden = true;
}
