/*
 * Local Advanced Programmable Interrupt Controller (LAPIC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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
#include "cmdline.hpp"
#include "ec.hpp"
#include "extern.hpp"
#include "lapic.hpp"
#include "msr.hpp"
#include "rcu.hpp"
#include "stc.hpp"
#include "stdio.hpp"
#include "string.hpp"
#include "vectors.hpp"

void Lapic::init (uint32 clk, uint32 rat)
{
    auto const apic_base { Msr::read (Msr::Register::IA32_APIC_BASE) };

#if 0   // FIXME
    Pd::kern.Space_mem::delreg (apic_base & ~OFFS_MASK);
#endif

    Hptp::current().update (MMAP_CPU_APIC, apic_base & ~OFFS_MASK, 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev());

    Msr::write (Msr::Register::IA32_APIC_BASE, apic_base | BIT (11));

    auto const svr { read (Register32::SVR) };
    if (!(svr & BIT (8)))
        write (Register32::SVR, svr | BIT (8));

    bool const dl { Cpu::feature (Cpu::FEAT_TSC_DEADLINE) && !Cmdline::nodl };

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
            set_lvt (Register32::LVT_TIMER, Delivery::DLV_FIXED, VEC_LVT + 0, dl * BIT (18));
    }

    write (Register32::TPR, 0x10);
    write (Register32::TMR_DCR, 0xb);

    Cpu::id = Cpu::find_by_apic_id (id());

    if ((Cpu::bsp = apic_base & BIT (8))) {

        memcpy (Hptp::map (0x1000, true), reinterpret_cast<void *>(Kmem::sym_to_virt (&__init_aps)), &__desc_gdt__ - &__init_aps);

        send_exc (0, Delivery::DLV_INIT);

        write (Register32::TMR_ICR, ~0U);

        auto const v1 { read (Register32::TMR_CCR) };
        auto const t1 { time() };
        Acpi::delay (10);
        auto const v2 { read (Register32::TMR_CCR) };
        auto const t2 { time() };

        auto const v { v1 - v2 };
        auto const t { t2 - t1 };
        auto const f { static_cast<uint64>(clk) * rat };

        ratio = dl ? 0 : f ? rat : static_cast<unsigned>((t + v / 2) / v);

        Stc::freq = f ? f : t * 100;

        trace (TRACE_INTR, "FREQ: %llu Hz (%s) Ratio:%u", Stc::freq, f ? "enumerated" : "measured", ratio);

        send_exc (1, Delivery::DLV_SIPI);
        Acpi::delay (1);
        send_exc (1, Delivery::DLV_SIPI);
    }

    write (Register32::TMR_ICR, 0);

    // Enforce ordering between the LVT MMIO write that enables TSC deadline mode and later WRMSRs to IA32_TSC_DEADLINE
    Barrier::fmb();

    trace (TRACE_INTR, "APIC: %#010llx ID:%#x VER:%#x SUP:%u LVT:%#x (%s Mode)", apic_base & ~OFFS_MASK, id(), version(), eoi_sup(), lvt_max(), ratio ? "OS" : "DL");
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
