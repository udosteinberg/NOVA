/*
 * Local Advanced Programmable Interrupt Controller (LAPIC)
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

#include "acpi.hpp"
#include "cmdline.hpp"
#include "ec.hpp"
#include "extern.hpp"
#include "lapic.hpp"
#include "msr.hpp"
#include "rcu.hpp"
#include "stdio.hpp"
#include "string.hpp"
#include "timeout.hpp"
#include "vectors.hpp"

void Lapic::init()
{
    auto apic_base = Msr::read (Msr::IA32_APIC_BASE);

#if 0   // FIXME
    Pd::kern.Space_mem::delreg (apic_base & ~PAGE_MASK);
#endif

    Hptp::current().update (CPU_LOCAL_APIC, apic_base & ~PAGE_MASK, 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::Cacheability::MEM_UC, Memattr::Shareability::NONE);

    Msr::write (Msr::IA32_APIC_BASE, apic_base | BIT (11));

    auto svr = read (Register32::SVR);
    if (!(svr & BIT (8)))
        write (Register32::SVR, svr | BIT (8));

    bool dl = Cpu::feature (Cpu::FEAT_TSC_DEADLINE) && !Cmdline::nodl;

    switch (lvt_max()) {
        default:
            set_lvt (Register32::LVT_THERM, Delivery::DLV_FIXED, VEC_LVT_THERM);
            FALLTHROUGH;
        case 4:
            set_lvt (Register32::LVT_PERFM, Delivery::DLV_FIXED, VEC_LVT_PERFM);
            FALLTHROUGH;
        case 3:
            set_lvt (Register32::LVT_ERROR, Delivery::DLV_FIXED, VEC_LVT_ERROR);
            FALLTHROUGH;
        case 2:
            set_lvt (Register32::LVT_LINT1, Delivery::DLV_NMI, 0);
            FALLTHROUGH;
        case 1:
            set_lvt (Register32::LVT_LINT0, Delivery::DLV_EXTINT, 0, BIT (16));
            FALLTHROUGH;
        case 0:
            set_lvt (Register32::LVT_TIMER, Delivery::DLV_FIXED, VEC_LVT_TIMER, dl ? BIT (18) : 0);
    }

    write (Register32::TPR, 0x10);
    write (Register32::TMR_DCR, 0xb);

    Cpu::id = Cpu::find_by_apic_id (id());

    if ((Cpu::bsp = apic_base & BIT (8))) {

        memcpy (Hptp::map (0x1000, true), reinterpret_cast<void *>(Kmem::sym_to_virt (&__init_aps)), &__desc_gdt__ - &__init_aps);

        send_ipi (0, 0, Delivery::DLV_INIT, Shorthand::EXC_SELF);

        write (Register32::TMR_ICR, ~0U);

        auto v1 = read (Register32::TMR_CCR);
        auto t1 = static_cast<uint32>(time());
        Acpi::delay (10);
        auto v2 = read (Register32::TMR_CCR);
        auto t2 = static_cast<uint32>(time());

        freq_tsc = (t2 - t1) / 10;
        freq_bus = (v1 - v2) / 10;

        trace (TRACE_INTR, "FREQ: TSC:%u kHz BUS:%u kHz", freq_tsc, freq_bus);

        send_ipi (0, 1, Delivery::DLV_SIPI, Shorthand::EXC_SELF);
        Acpi::delay (1);
        send_ipi (0, 1, Delivery::DLV_SIPI, Shorthand::EXC_SELF);
    }

    write (Register32::TMR_ICR, 0);

    trace (TRACE_INTR, "APIC: %#010llx ID:%#x VER:%#x SUP:%u LVT:%#x (%s Mode)", apic_base & ~PAGE_MASK, id(), version(), eoi_sup(), lvt_max(), freq_bus ? "OS" : "DL");
}

void Lapic::send_ipi (unsigned cpu, unsigned v, Delivery d, Shorthand s)
{
    while (EXPECT_FALSE (read (Register32::ICR) & BIT (12)))
        pause();

    write (Register64::ICR, static_cast<uint64>(Cpu::apic_id[cpu]) << 56 | static_cast<uint32>(s) | BIT (14) | static_cast<uint32>(d) | v);
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
    bool expired = (freq_bus ? read (Register32::TMR_CCR) : Msr::read (Msr::IA32_TSC_DEADLINE)) == 0;
    if (expired)
        Timeout::check();

    Rcu::update();
}
