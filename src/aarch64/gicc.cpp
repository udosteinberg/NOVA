/*
 * Generic Interrupt Controller: Physical CPU Interface (GICC/ICC)
 *
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

#include "assert.hpp"
#include "gicc.hpp"
#include "ptab_hpt.hpp"
#include "stdio.hpp"

void Gicc::init()
{
    init_mode();

    if (Cpu::bsp)
        mmap_mmio();

    switch (mode) {
        case Mode::MMIO: init_mmio(); break;
        case Mode::REGS: init_regs(); break;
    }
}

void Gicc::mmap_mmio()
{
    if (phys) {

        // FIXME
        constexpr auto offs = Board::gic[2].size == 0x20000 ? 0xf000 : 0;

        // Map MMIO region
        if (mode == Mode::MMIO)
            for (unsigned i = 0; i < 2; i++)
                Hptp::master.update (MMAP_GLB_GICC + i * PAGE_SIZE, phys + offs + i * PAGE_SIZE, 0,
                                     Paging::Permissions (Paging::G | Paging::W | Paging::R),
                                     Memattr::Cacheability::DEV, Memattr::Shareability::NONE);
    }

    else if (mode == Mode::MMIO)
        trace (TRACE_INTR, "GICC: MMIO unavailable. This is BAD!");
}

void Gicc::init_mmio()
{
    // Disable interrupt signaling
    write (Register32::CTLR, 0);

    write (Register32::PMR, 0xff);
    write (Register32::BPR, 0x7);

    // Enable interrupt signaling
    write (Register32::CTLR, BIT (9) | BIT (6) | BIT (5) | BIT (0));

    auto iidr = read (Register32::IIDR);
    auto arch = iidr >> 16 & BIT_RANGE (3, 0);

    trace (TRACE_INTR, "GICC: %#010llx v%u r%up%u Impl:%#x Prod:%#x",
           phys, arch, arch, iidr >> 12 & BIT_RANGE (3, 0), iidr & BIT_RANGE (11, 0), iidr >> 20);
}

void Gicc::init_regs()
{
    set_el1_pmr     (0xff);
    set_el1_bpr1    (0x7);
    set_el1_igrpen1 (BIT (0));
    set_el1_ctlr    (BIT (1));

    // Ensure system register writes executed
    Barrier::isb();

    trace (TRACE_INTR, "GICC: REGS");
}

void Gicc::init_mode()
{
    if (!Cpu::feature (Cpu::Cpu_feature::GIC))
        return;

    // Disable IRQ/FIQ bypass and enable system registers
    set_el2_sre (get_el2_sre() | BIT_RANGE (2, 0));

    // Ensure system register write executed
    Barrier::isb();

    if (get_el2_sre() & BIT (0))
        mode = Mode::REGS;
}

void Gicc::send_cpu (unsigned sgi, unsigned cpu)
{
    assert (sgi < NUM_SGI && mode == Mode::REGS);

    uint64 affinity = Cpu::remote_affinity (cpu);

    send_sgi (sgi << 24 | (affinity & BIT_RANGE (31, 24)) << 24 |  // Aff3
                          (affinity & BIT_RANGE (23, 16)) << 16 |  // Aff2
                          (affinity & BIT_RANGE (15,  8)) <<  8 |  // Aff1
                      BIT (affinity & BIT_RANGE ( 3,  0)));        // Aff0
}

void Gicc::send_exc (unsigned sgi)
{
    assert (sgi < NUM_SGI && mode == Mode::REGS);

    send_sgi (BIT64 (40) | sgi << 24);
}
