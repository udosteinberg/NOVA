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
    auto apic_base = Msr::read (Msr::IA32_APIC_BASE);

#if 0   // FIXME
    Pd::kern.Space_mem::delreg (apic_base & ~OFFS_MASK);
#endif

    Hptp (Hpt::current()).update (MMAP_CPU_APIC, 0, Hpt::HPT_NX | Hpt::HPT_G | Hpt::HPT_UC | Hpt::HPT_W | Hpt::HPT_P, apic_base & ~OFFS_MASK);

    Msr::write (Msr::IA32_APIC_BASE, apic_base | BIT (11));

    auto svr = read (Register32::SVR);
    if (!(svr & BIT (8)))
        write (Register32::SVR, svr | BIT (8));

    bool dl = Cpu::feature (Cpu::FEAT_TSC_DEADLINE) && !Cmdline::nodl;

    switch (lvt_max()) {
        default:
            set_lvt (Register32::LVT_THERM, Delivery::DLV_FIXED, VEC_LVT_THERM);
            [[fallthrough]];
        case 4:
            set_lvt (Register32::LVT_PERFM, Delivery::DLV_FIXED, VEC_LVT_PERFM);
            [[fallthrough]];
        case 3:
            set_lvt (Register32::LVT_ERROR, Delivery::DLV_FIXED, VEC_LVT_ERROR);
            [[fallthrough]];
        case 2:
            set_lvt (Register32::LVT_LINT1, Delivery::DLV_NMI, 0);
            [[fallthrough]];
        case 1:
            set_lvt (Register32::LVT_LINT0, Delivery::DLV_EXTINT, 0, BIT (16));
            [[fallthrough]];
        case 0:
            set_lvt (Register32::LVT_TIMER, Delivery::DLV_FIXED, VEC_LVT_TIMER, dl * BIT (18));
    }

    write (Register32::TPR, 0x10);
    write (Register32::TMR_DCR, 0xb);

    Cpu::id = Cpu::find_by_apic_id (id());

    if ((Cpu::bsp = apic_base & BIT (8))) {

        memcpy (Hpt::remap (0x1000), reinterpret_cast<void *>(Kmem::sym_to_virt (&__init_aps)), &__desc_gdt__ - &__init_aps);

        send_exc (0, Delivery::DLV_INIT);

        write (Register32::TMR_ICR, ~0U);

        auto v1 = read (Register32::TMR_CCR);
        auto t1 = time();
        Acpi::delay (10);
        auto v2 = read (Register32::TMR_CCR);
        auto t2 = time();

        auto v = v1 - v2;
        auto t = t2 - t1;
        auto f = static_cast<uint64>(clk) * rat;

        ratio = dl ? 0 : f ? rat : static_cast<unsigned>((t + v / 2) / v);

        Stc::freq = f ? f : t * 100;

        trace (TRACE_INTR, "FREQ: %llu Hz (%s) Ratio:%u", Stc::freq, f ? "provided" : "measured", ratio);

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
    bool expired = (ratio ? read (Register32::TMR_CCR) : Msr::read (Msr::IA32_TSC_DEADLINE)) == 0;
    if (expired)
        Stc::interrupt();

    Rcu::update();
}

void Lapic::lvt_vector (unsigned vector)
{
    unsigned lvt = vector - VEC_LVT;

    switch (vector) {
        case VEC_LVT_TIMER: timer_handler(); break;
        case VEC_LVT_ERROR: error_handler(); break;
        case VEC_LVT_PERFM: perfm_handler(); break;
        case VEC_LVT_THERM: therm_handler(); break;
    }

    eoi();

    Counter::loc[lvt].inc();
}

void Lapic::ipi_vector (unsigned vector)
{
    unsigned ipi = vector - VEC_IPI;

    switch (vector) {
        case VEC_IPI_RRQ: Sc::rrq_handler(); break;
        case VEC_IPI_RKE: Sc::rke_handler(); break;
    }

    eoi();

    Counter::req[ipi].inc();
}
