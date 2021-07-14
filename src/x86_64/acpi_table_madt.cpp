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
#include "ioapic.hpp"
#include "pic.hpp"

void Acpi_table_madt::Controller_lapic::parse() const
{
    if (Cpu::count < NUM_CPU && flags & (Flags::ONLINE_CAPABLE | Flags::ENABLED)) {
        Cpu::acpi_id[Cpu::count]   = acpi_uid;
        Cpu::apic_id[Cpu::count++] = id;
    }
}

void Acpi_table_madt::Controller_ioapic::parse() const
{
    auto ioapic = new Ioapic (phys, id, gsi);

    for (auto i = ioapic->mre() + 1, g = gsi; i-- && g < NUM_GSI; g++)
        Interrupt::int_table[g].ioapic = ioapic;
}

void Acpi_table_madt::parse() const
{
    if (flags & Flags::PCAT_COMPAT)
        Pic::exists = true;

    auto addr = reinterpret_cast<uintptr_t>(this);

    for (auto a = addr + sizeof (*this); a < addr + length; ) {

        auto c = reinterpret_cast<Controller const *>(a);

        switch (c->type) {
            case Controller::LAPIC:  static_cast<Controller_lapic  const *>(c)->parse(); break;
            case Controller::IOAPIC: static_cast<Controller_ioapic const *>(c)->parse(); break;
            default: break;
        }

        a += c->length;
    }
}
