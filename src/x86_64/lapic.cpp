/*
 * Local Advanced Programmable Interrupt Controller (LAPIC)
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

#include "acpi.hpp"
#include "barrier.hpp"
#include "ec.hpp"
#include "lapic.hpp"
#include "msr.hpp"
#include "rcu.hpp"
#include "space_hst.hpp"
#include "stc.hpp"
#include "stdio.hpp"
#include "txt.hpp"
#include "vectors.hpp"

void Lapic::init (uint32_t clk, uint32_t rat)
{
    auto const apic_base { Msr::read (Msr::Register::IA32_APIC_BASE) };

    if (!Acpi::resume) {

        // Reserve MMIO region
        Space_hst::user_access (apic_base & ~OFFS_MASK (0), PAGE_SIZE (0), false);

        // Map MMIO region
        Hptp::current().update (MMAP_CPU_APIC, apic_base & ~OFFS_MASK (0), 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev());

        // Determine CPU number from the APIC ID of the currently enabled interface
        Cpu::id = lookup (apic_base & BIT (10) ? read_x2apic (Register32::IDR) : read_legacy (Register32::IDR) >> 24);
    }

    // HW enable
    Msr::write (Msr::Register::IA32_APIC_BASE, apic_base | BIT (11) | BIT (10) * x2apic);

    // SW enable
    write (Register32::SVR, read (Register32::SVR) | BIT (8));

    bool const dl { Cpu::feature (Cpu::Feature::TSC_DEADLINE) };

    switch (lvt_max()) {
        default:
            set_lvt (Register32::LVT_THERM, Delivery::DLV_FIXED, VEC_LVT + 3);
            [[fallthrough]];
        case 4:
            set_lvt (Register32::LVT_PERFM, Delivery::DLV_FIXED, VEC_LVT + 2);
            [[fallthrough]];
        case 3:
            set_lvt (Register32::LVT_ERROR, Delivery::DLV_FIXED, VEC_LVT + 1);
            [[fallthrough]];
        case 2:
            set_lvt (Register32::LVT_LINT1, Delivery::DLV_NMI, 0);
            [[fallthrough]];
        case 1:
            set_lvt (Register32::LVT_LINT0, Delivery::DLV_EXTINT, 0, BIT (16));
            [[fallthrough]];
        case 0:
            set_lvt (Register32::LVT_TIMER, Delivery::DLV_FIXED, VEC_LVT + 0, BIT (18) * dl);
    }

    write (Register32::TPR, 0x10);
    write (Register32::TMR_DCR, 0xb);

    if ((Cpu::bsp = apic_base & BIT (8))) {

        if (!Txt::launched)
            send_exc (0, Delivery::DLV_INIT);

        write (Register32::TMR_ICR, ~0U);

        auto const c1 { read (Register32::TMR_CCR) };
        auto const t1 { time() };
        Acpi_fixed::delay (10);
        auto const c2 { read (Register32::TMR_CCR) };
        auto const t2 { time() };

        auto const c { c1 - c2 };
        auto const t { t2 - t1 };
        auto const f { static_cast<uint64_t>(clk) * rat };

        ratio = dl ? 0 : f ? rat : static_cast<unsigned>((t + c / 2) / c);

        Stc::freq = f ? f : t * 100;

        trace (TRACE_INTR, "FREQ: %lu Hz (%s) Ratio:%u", Stc::freq, f ? "enumerated" : "measured", ratio);

        if (!Txt::launched) {
            send_exc (Acpi::sipi >> PAGE_BITS, Delivery::DLV_SIPI);
            Acpi_fixed::delay (1);
            send_exc (Acpi::sipi >> PAGE_BITS, Delivery::DLV_SIPI);
        }
    }

    write (Register32::TMR_ICR, 0);

    // Enforce ordering between the LVT MMIO write that enables TSC deadline mode and later WRMSRs to IA32_TSC_DEADLINE
    Barrier::fmb();

    trace (TRACE_INTR, "APIC: LOC:%#04x VER:%#x SUP:%u LVT:%#x (x%sAPIC %s Mode)", id[Cpu::id], version(), eoi_sup(), lvt_max(), x2apic ? "2" : "", ratio ? "OS" : "DL");
}

void Lapic::therm_handler() {}

void Lapic::perfm_handler() {}

void Lapic::error_handler()
{
    write (Register32::ESR, 0);
    write (Register32::ESR, 0);
}

void Lapic::timer_handler()
{
    bool const expired { (ratio ? read (Register32::TMR_CCR) : Msr::read (Msr::Register::IA32_TSC_DEADLINE)) == 0 };

    if (expired)
        Stc::interrupt();

    Rcu::update();
}
