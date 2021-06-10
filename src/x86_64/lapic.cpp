/*
 * Local Advanced Programmable Interrupt Controller (Local APIC)
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

void Lapic::init()
{
    Paddr apic_base = static_cast<Paddr>(Msr::read (Msr::IA32_APIC_BASE));

    Pd::kern.Space_mem::delreg (apic_base & ~OFFS_MASK);
    Hptp (Hpt::current()).update (MMAP_CPU_APIC, 0, Hpt::HPT_NX | Hpt::HPT_G | Hpt::HPT_UC | Hpt::HPT_W | Hpt::HPT_P, apic_base & ~OFFS_MASK);

    Msr::write (Msr::IA32_APIC_BASE, apic_base | 0x800);

    uint32 svr = read (LAPIC_SVR);
    if (!(svr & 0x100))
        write (LAPIC_SVR, svr | 0x100);

    bool dl = Cpu::feature (Cpu::FEAT_TSC_DEADLINE) && !Cmdline::nodl;

    switch (lvt_max()) {
        default:
            set_lvt (LAPIC_LVT_THERM, DLV_FIXED, VEC_LVT_THERM);
            [[fallthrough]];
        case 4:
            set_lvt (LAPIC_LVT_PERFM, DLV_FIXED, VEC_LVT_PERFM);
            [[fallthrough]];
        case 3:
            set_lvt (LAPIC_LVT_ERROR, DLV_FIXED, VEC_LVT_ERROR);
            [[fallthrough]];
        case 2:
            set_lvt (LAPIC_LVT_LINT1, DLV_NMI, 0);
            [[fallthrough]];
        case 1:
            set_lvt (LAPIC_LVT_LINT0, DLV_EXTINT, 0, 1U << 16);
            [[fallthrough]];
        case 0:
            set_lvt (LAPIC_LVT_TIMER, DLV_FIXED, VEC_LVT_TIMER, dl ? 2U << 17 : 0);
    }

    write (LAPIC_TPR, 0x10);
    write (LAPIC_TMR_DCR, 0xb);

    Cpu::id = Cpu::find_by_apic_id (id());

    if ((Cpu::bsp = apic_base & 0x100)) {

        memcpy (Hpt::remap (0x1000), reinterpret_cast<void *>(Kmem::sym_to_virt (&__init_aps)), &__desc_gdt__ - &__init_aps);

        send_ipi (0, 0, DLV_INIT, DSH_EXC_SELF);

        write (LAPIC_TMR_ICR, ~0U);

        auto v1 = read (LAPIC_TMR_CCR);
        auto t1 = time();
        Acpi::delay (10);
        auto v2 = read (LAPIC_TMR_CCR);
        auto t2 = time();

        auto v = v1 - v2;
        auto t = t2 - t1;

        ratio = dl ? 0 : static_cast<unsigned>((t + v / 2) / v);

        Stc::freq = t * 100;

        trace (TRACE_INTR, "FREQ: TSC:%llu Hz Ratio:%u", Stc::freq, ratio);

        send_ipi (0, 1, DLV_SIPI, DSH_EXC_SELF);
        Acpi::delay (1);
        send_ipi (0, 1, DLV_SIPI, DSH_EXC_SELF);
    }

    write (LAPIC_TMR_ICR, 0);

    trace (TRACE_INTR, "APIC:%#lx ID:%#x VER:%#x LVT:%#x (%s Mode)", apic_base & ~OFFS_MASK, id(), version(), lvt_max(), ratio ? "OS" : "DL");
}

void Lapic::send_ipi (unsigned cpu, unsigned vector, Delivery_mode dlv, Shorthand dsh)
{
    while (EXPECT_FALSE (read (LAPIC_ICR_LO) & 1U << 12))
        pause();

    write (LAPIC_ICR_HI, Cpu::apic_id[cpu] << 24);
    write (LAPIC_ICR_LO, dsh | 1U << 14 | dlv | vector);
}

void Lapic::therm_handler() {}

void Lapic::perfm_handler() {}

void Lapic::error_handler()
{
    write (LAPIC_ESR, 0);
    write (LAPIC_ESR, 0);
}

void Lapic::timer_handler()
{
    bool expired = (ratio ? read (LAPIC_TMR_CCR) : Msr::read (Msr::IA32_TSC_DEADLINE)) == 0;
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
