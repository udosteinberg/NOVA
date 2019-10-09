/*
 * Generic Interrupt Controller: Hypervisor Interface (GICH/ICH)
 *
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "gich.hpp"
#include "hpt.hpp"
#include "pd.hpp"
#include "stdio.hpp"

unsigned Gich::num_apr, Gich::num_lr;

void Gich::init()
{
    // Reserve MMIO region even if we use REGS
    if (Cpu::bsp)
        Pd::remove_mem_user (GICH_BASE, GICH_SIZE);

    switch (Gicc::mode) {
        case Gicc::Mode::REGS: init_regs(); break;
        case Gicc::Mode::MMIO: init_mmio(); break;
    }
}

void Gich::init_regs()
{
    auto vtr = get_el2_vtr();

    num_apr = 1U << ((vtr >> 26 & 0x7) + 1) >> 5;
    num_lr  = static_cast<unsigned>((vtr & 0x1f) + 1);

    trace (TRACE_INTR, "GICH: REGS APR:%u LR:%u", num_apr, num_lr);
}

void Gich::init_mmio()
{
    if (!GICH_SIZE)
        return;

    if (Cpu::bsp)
        Hptp::master.update (DEV_GLOBL_GICH, GICH_BASE, 0,
                             Paging::Permissions (Paging::R | Paging::W | Paging::G),
                             Memattr::Cacheability::DEV, Memattr::Shareability::NONE);

    auto vtr = read (Register32::VTR);

    num_apr = 1U << ((vtr >> 26 & 0x7) + 1) >> 5;
    num_lr  = (vtr & 0x1f) + 1;

    trace (TRACE_INTR, "GICH: %#010x APR:%u LR:%u", GICH_BASE, num_apr, num_lr);
}
