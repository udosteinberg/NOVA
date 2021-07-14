/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

bool Acpi_table_madt::Controller_lapic::parse() const
{
    // Abort if length is below minimum
    if (len < sizeof (*this)) [[unlikely]]
        return false;

    // The CPU is usable
    if (flags & BIT_RANGE (1, 0)) [[likely]]
        Cpu::allocate (id);

    return true;
}

bool Acpi_table_madt::Controller_x2apic::parse() const
{
    // Abort if length is below minimum
    if (len < sizeof (*this)) [[unlikely]]
        return false;

    // The CPU is usable
    if (flags & BIT_RANGE (1, 0)) [[likely]]
        Cpu::allocate (id);

    return true;
}

bool Acpi_table_madt::Controller_ioapic::parse() const
{
    // Abort if length is below minimum
    if (len < sizeof (*this)) [[unlikely]]
        return false;

    auto const ioapic { new Ioapic { phys, id, gsi } };
    if (!ioapic) [[unlikely]]
        panic ("IOAPIC allocation failed");

    Interrupt::num_pin = max (Interrupt::num_pin, static_cast<uint16_t>(gsi + ioapic->mre() + 1));

    return true;
}

bool Acpi_table_madt::parse() const
{
    if (flags & BIT (0))
        Pic::exists = true;

    using list_t = Controller;
    bool       ret { true };
    auto       ptr { reinterpret_cast<uintptr_t>(this + 1) };
    auto const end { reinterpret_cast<uintptr_t>(this) + table.header.length };

    while (ptr + sizeof (list_t) <= end) {

        auto const s { std::start_lifetime_as<list_t const> (ptr) };
        auto const l { s->len };

        trace (TRACE_FIRM | TRACE_PARSE, "MADT: Type:%#x Len:%u", std::to_underlying (s->type()), unsigned { l });

        // Abort if structure size is below minimum or exceeds list bounds
        if (l < sizeof (list_t) || (ptr += l) > end) [[unlikely]]
            break;

        switch (s->type()) {
            case Controller::Type::LAPIC:  ret &= std::start_lifetime_as<Controller_lapic  const> (s)->parse(); break;
            case Controller::Type::IOAPIC: ret &= std::start_lifetime_as<Controller_ioapic const> (s)->parse(); break;
            case Controller::Type::X2APIC: ret &= std::start_lifetime_as<Controller_x2apic const> (s)->parse(); break;
            default: break;
        }
    }

    return ret && end == ptr;
}
