/*
 * Advanced Configuration and Power Interface (ACPI)
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
#include "acpi_madt.h"
#include "cmdline.h"
#include "gsi.h"
#include "ioapic.h"
#include "lapic.h"

void Acpi_table_madt::parse() const
{
    if (!Cmdline::nomp)
        parse_entry (Acpi_apic::LAPIC, &Acpi_table_madt::parse_lapic);

    parse_entry (Acpi_apic::IOAPIC, &Acpi_table_madt::parse_ioapic);
    parse_entry (Acpi_apic::INTR_OVERRIDE, &Acpi_table_madt::parse_intr_override);
}

void Acpi_table_madt::parse_entry (Acpi_apic::Type type, void (*handler)(Acpi_apic const *)) const
{
    for (Acpi_apic const *acpi_apic = apic;
         acpi_apic < reinterpret_cast<Acpi_apic *>(reinterpret_cast<mword>(this) + length);
         acpi_apic = reinterpret_cast<Acpi_apic *>(reinterpret_cast<mword>(acpi_apic) + acpi_apic->length))
        if (acpi_apic->type == type)
            (*handler)(acpi_apic);
}

void Acpi_table_madt::parse_lapic (Acpi_apic const *acpi_apic)
{
    Acpi_lapic const *acpi_lapic = static_cast<Acpi_lapic const *>(acpi_apic);

    if (acpi_lapic->enabled)
        Lapic::mask_cpu |= 1UL << acpi_lapic->id;
}

void Acpi_table_madt::parse_ioapic (Acpi_apic const *acpi_apic)
{
    Acpi_ioapic const *acpi_ioapic = static_cast<Acpi_ioapic const *>(acpi_apic);

    Ioapic *ioapic = new Ioapic (acpi_ioapic->phys, acpi_ioapic->gsi, acpi_ioapic->id);

    unsigned gsi = acpi_ioapic->gsi;
    unsigned max = ioapic->irt_max();

    for (unsigned short i = 0; i <= max && gsi < NUM_GSI; i++, gsi++)
        Gsi::gsi_table[gsi].ioapic = ioapic;
}

void Acpi_table_madt::parse_intr_override (Acpi_apic const *acpi_apic)
{
    Acpi_intr_override const *intr = static_cast<Acpi_intr_override const *>(acpi_apic);

    unsigned irq = intr->irq;
    unsigned gsi = intr->gsi;

    if (EXPECT_FALSE (gsi >= NUM_GSI || irq >= NUM_IRQ || intr->bus))
        return;

    Gsi::irq_table[irq] = gsi;

    Gsi::gsi_table[gsi].pol = intr->flags.pol == Acpi_inti::POL_LOW   || (intr->flags.pol == Acpi_inti::POL_CONFORMING && irq == Acpi::irq);
    Gsi::gsi_table[gsi].trg = intr->flags.trg == Acpi_inti::TRG_LEVEL || (intr->flags.trg == Acpi_inti::TRG_CONFORMING && irq == Acpi::irq);

    if (irq == Acpi::irq)
        sci_overridden = true;
}
