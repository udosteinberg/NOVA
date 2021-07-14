/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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
#include "lapic.hpp"
#include "lowlevel.hpp"
#include "pic.hpp"
#include "stdio.hpp"
#include "util.hpp"

void Acpi_table_madt::Controller_lapic::parse() const
{
    // The CPU is usable
    if (flags & BIT_RANGE (1, 0)) [[likely]]
        Cpu::allocate (id);
}

void Acpi_table_madt::Controller_x2apic::parse() const
{
    // The CPU is usable
    if (flags & BIT_RANGE (1, 0)) [[likely]]
        Cpu::allocate (id);
}

void Acpi_table_madt::Controller_ioapic::parse() const
{
    auto const ioapic { new Ioapic { phys, id, gsi } };
    if (!ioapic) [[unlikely]]
        panic ("IOAPIC allocation failed");

    Interrupt::num_pin = max (Interrupt::num_pin, static_cast<uint16_t>(gsi + ioapic->mre() + 1));
}

void Acpi_table_madt::parse() const
{
    if (flags & BIT (0))
        Pic::exists = true;

    for (auto ptr { reinterpret_cast<uintptr_t>(this + 1) }; ptr < reinterpret_cast<uintptr_t>(this) + table.header.length; ) {

        auto const c { reinterpret_cast<Controller const *>(ptr) };

        switch (c->type()) {
            case Controller::Type::LAPIC:  static_cast<Controller_lapic  const *>(c)->parse(); break;
            case Controller::Type::IOAPIC: static_cast<Controller_ioapic const *>(c)->parse(); break;
            case Controller::Type::X2APIC: static_cast<Controller_x2apic const *>(c)->parse(); break;
            default: break;
        }

        ptr += c->length;
    }
}
