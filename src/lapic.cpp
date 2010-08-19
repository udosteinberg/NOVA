/*
 * Local Advanced Programmable Interrupt Controller (Local APIC)
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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
#include "ec.h"
#include "lapic.h"
#include "msr.h"
#include "rcu.h"

unsigned    Lapic::freq_tsc;
unsigned    Lapic::freq_bus;
unsigned    Lapic::mask_cpu;

void Lapic::init()
{
    Paddr apic_base = Msr::read<Paddr>(Msr::IA32_APIC_BASE);

    Cpu::bsp = apic_base & 0x100;

    Pd::kern.insert_local (LAPIC_ADDR, 0,
                           Ptab::Attribute (Ptab::ATTR_NOEXEC      |
                                            Ptab::ATTR_GLOBAL      |
                                            Ptab::ATTR_UNCACHEABLE |
                                            Ptab::ATTR_WRITABLE    |
                                            Ptab::ATTR_PRESENT),
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

void Lapic::wake_ap()
{
    for (unsigned i = 0; i < NUM_CPU; i++) {

        if (id() != i) {

            if (!(mask_cpu & 1UL << i))
                continue;

            send_ipi (i, DST_PHYSICAL, DLV_INIT, 0);
            Acpi::delay (10);
            send_ipi (i, DST_PHYSICAL, DLV_SIPI, 1);
            Acpi::delay (1);
            send_ipi (i, DST_PHYSICAL, DLV_SIPI, 1);
        }

        Cpu::boot_count++;
    }
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
        Cpu::hazard |= HZD_SCHED;

    if (Rcu::process_callbacks())
        Cpu::hazard |= HZD_SCHED;
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

    Counter::print (++Counter::lvt[lvt], Console_vga::COLOR_LIGHT_BLUE, lvt + SPN_LVT);
}

void Lapic::ipi_vector (unsigned vector)
{
    unsigned ipi = vector - VEC_IPI;

    switch (vector) {
        case VEC_IPI_RRQ: Sc::rrq_handler(); break;
        case VEC_IPI_TLB: Sc::tlb_handler(); break;
    }

    eoi();

    Counter::print (++Counter::ipi[ipi], Console_vga::COLOR_LIGHT_GREEN, ipi + SPN_IPI);
}
