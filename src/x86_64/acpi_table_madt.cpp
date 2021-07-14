/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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
    if (EXPECT_FALSE (Cpu::count == NUM_CPU)) {
        trace (TRACE_FIRM, "STOP: Platform has more than %u CPUs", NUM_CPU);
        shutdown();
    }

    if (flags & (Flags::ONLINE_CAPABLE | Flags::ENABLED))
        Lapic::id[Cpu::count++] = id;
}

void Acpi_table_madt::Controller_x2apic::parse() const
{
    if (EXPECT_FALSE (Cpu::count == NUM_CPU)) {
        trace (TRACE_FIRM, "STOP: Platform has more than %u CPUs", NUM_CPU);
        shutdown();
    }

    if (flags & (Flags::ONLINE_CAPABLE | Flags::ENABLED))
        Lapic::id[Cpu::count++] = id;
}

void Acpi_table_madt::Controller_ioapic::parse() const
{
    auto const ioapic { new Ioapic (phys, id, gsi) };

    auto const gsi_end { min (gsi + ioapic->mre() + 1, static_cast<unsigned>(NUM_GSI)) };

    for (unsigned i { gsi }; i < gsi_end; i++)
        Interrupt::int_table[i].ioapic = ioapic;

    Interrupt::pin = max (Interrupt::pin, gsi_end);
}

void Acpi_table_madt::parse() const
{
    auto const len { table.header.length };
    auto const ptr { reinterpret_cast<uintptr_t>(this) };

    if (EXPECT_FALSE (len < sizeof (*this)))
        return;

    if (flags & Flags::PCAT_COMPAT)
        Pic::exists = true;

    for (auto p { ptr + sizeof (*this) }; p < ptr + len; ) {

        auto const c { reinterpret_cast<Controller const *>(p) };

        switch (c->type) {
            case Controller::LAPIC:  static_cast<Controller_lapic  const *>(c)->parse(); break;
            case Controller::IOAPIC: static_cast<Controller_ioapic const *>(c)->parse(); break;
            case Controller::X2APIC: static_cast<Controller_x2apic const *>(c)->parse(); break;
            default: break;
        }

        p += c->length;
    }
}
