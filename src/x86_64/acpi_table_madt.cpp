/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#include "acpi_table_madt.hpp"
#include "cpu.hpp"
#include "interrupt.hpp"
#include "io.hpp"
#include "ioapic.hpp"
#include "vectors.hpp"

void Acpi_table_madt::parse() const
{
    parse_entry (Acpi_apic::LAPIC,  &parse_lapic);
    parse_entry (Acpi_apic::IOAPIC, &parse_ioapic);

    if (flags & BIT (0)) {
        Io::out<uint8>(0x20, BIT (4) | BIT (0));    // ICW1
        Io::out<uint8>(0x21, VEC_GSI);              // ICW2
        Io::out<uint8>(0x21, BIT (2));              // ICW3
        Io::out<uint8>(0x21, BIT (0));              // ICW4
        Io::out<uint8>(0x21, BIT_RANGE (7, 0));     // OCW1
    }
}

void Acpi_table_madt::parse_entry (Acpi_apic::Type type, void (*handler)(Acpi_apic const *)) const
{
    for (Acpi_apic const *ptr = apic; ptr < reinterpret_cast<Acpi_apic *>(reinterpret_cast<uintptr_t>(this) + length); ptr = reinterpret_cast<Acpi_apic *>(reinterpret_cast<uintptr_t>(ptr) + ptr->length))
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
        Interrupt::int_table[gsi].ioapic = ioapic;
}
