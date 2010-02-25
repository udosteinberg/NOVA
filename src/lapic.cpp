/*
 * Local Advanced Programmable Interrupt Controller (Local APIC)
 *
 * Copyright (C) 2006-2009, Udo Steinberg <udo@hypervisor.org>
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

#include "acpi.h"
#include "counter.h"
#include "ec.h"
#include "lapic.h"
#include "pd.h"
#include "stdio.h"
#include "vectors.h"

unsigned    Lapic::freq_tsc;
unsigned    Lapic::freq_bus;
uint32      Lapic::present[8];

void Lapic::init()
{
    Paddr apic_base = Msr::read<Paddr>(Msr::IA32_APIC_BASE);

    Cpu::bsp = apic_base & 0x100;

    Pd::kern.insert_local (LAPIC_ADDR, 0,
                           Ptab::Attribute (Ptab::ATTR_NOEXEC      |
                                            Ptab::ATTR_GLOBAL      |
                                            Ptab::ATTR_UNCACHEABLE |
                                            Ptab::ATTR_WRITABLE),
                           apic_base & ~PAGE_MASK);

    Msr::write (Msr::IA32_APIC_BASE, apic_base | 0x800);

    uint32 svr = read (LAPIC_SVR);
    if (!(svr & 0x100))
        write (LAPIC_SVR, svr | 0x100);

    switch (lvt_max()) {
        default:
            set_lvt (LAPIC_LVT_THERM, VEC_LVT_THERM, DLV_FIXED);
        case 4:
            set_lvt (LAPIC_LVT_PERFM, VEC_LVT_PERFM, DLV_FIXED);
        case 3:
            set_lvt (LAPIC_LVT_ERROR, VEC_LVT_ERROR, DLV_FIXED);
        case 2:
            set_lvt (LAPIC_LVT_LINT1, 0, DLV_NMI);
        case 1:
            set_lvt (LAPIC_LVT_LINT0, 0, DLV_EXTINT, MASKED);
        case 0:
            set_lvt (LAPIC_LVT_TIMER, VEC_LVT_TIMER, DLV_FIXED, UNMASKED, ONESHOT);
    }

    write (LAPIC_TPR, 0x10);
    write (LAPIC_TMR_DCR, DIVIDE_BY_1);
    write (LAPIC_TMR_ICR, ~0u);

    set_ldr (1u << Cpu::id);
    set_dfr (LAPIC_FLAT);

    if (Cpu::bsp)
        calibrate();

    write (LAPIC_TMR_ICR, 0);

    trace (TRACE_APIC, "APIC:%#lx ID:%#x VER:%#x LVT:%#x",
           apic_base & ~PAGE_MASK, id(), version(), lvt_max());
}

void Lapic::calibrate()
{
    uint32 v1 = read (LAPIC_TMR_CCR);
    uint32 t1 = static_cast<uint32>(rdtsc());
    Acpi::delay (250);
    uint32 v2 = read (LAPIC_TMR_CCR);
    uint32 t2 = static_cast<uint32>(rdtsc());

    freq_tsc = (t2 - t1) / 250;
    freq_bus = (v1 - v2) / 250;

    trace (TRACE_CPU, "TSC:%u kHz BUS:%u kHz", freq_tsc, freq_bus);
}

void Lapic::send_ipi (unsigned cpu, Destination_mode dst, Delivery_mode dlv, unsigned vector)
{
    while (EXPECT_FALSE (read (LAPIC_ICR_LO) & STS_PENDING))
        pause();

    write (LAPIC_ICR_HI, cpu << 24);
    write (LAPIC_ICR_LO, dst | dlv | vector);
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
    if (!read (LAPIC_TMR_CCR))
        Cpu::hazard |= Cpu::HZD_SCHED;
}

void Lapic::lvt_vector (unsigned vector)
{
    unsigned lvt = vector - VEC_LVT;

    switch (lvt) {
        case 0: timer_handler(); break;
        case 3: error_handler(); break;
        case 4: perfm_handler(); break;
        case 5: therm_handler(); break;
    }

    eoi();

    Counter::count (Counter::lvt[lvt], Console_vga::COLOR_LIGHT_BLUE, 5 + vector - NUM_EXC);
}

void Lapic::ipi_vector (unsigned vector)
{
    unsigned ipi = vector - VEC_IPI;

    switch (ipi) {
        case 0: Sc::remote_enqueue_handler(); break;
    }

    eoi();

    Counter::count (Counter::ipi[ipi], Console_vga::COLOR_LIGHT_GREEN, 5 + vector - NUM_EXC);
}
