/*
 * Generic Interrupt Controller: Hypervisor Interface (GICH/ICH)
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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
#include "pd_kern.hpp"
#include "stdio.hpp"

void Gich::init()
{
    if (Cpu::bsp)
        mmap_mmio();

    switch (Gicc::mode) {
        case Gicc::Mode::MMIO: init_mmio(); break;
        case Gicc::Mode::REGS: init_regs(); break;
    }
}

void Gich::mmap_mmio()
{
    if (phys) {

        // FIXME
        constexpr auto size = Board::gic[3].size;

        // Reserve MMIO region
        Pd_kern::remove_user_mem (phys, size);

        // Map MMIO region
        if (Gicc::mode == Gicc::Mode::MMIO)
            Hptp::master.update (DEV_GLOBL_GICH, phys, 0,
                                 Paging::Permissions (Paging::G | Paging::W | Paging::R),
                                 Memattr::Cacheability::DEV, Memattr::Shareability::NONE);
    }

    else if (Gicc::mode == Gicc::Mode::MMIO)
        trace (TRACE_INTR, "GICH: MMIO unavailable. This is BAD!");
}

void Gich::init_mmio()
{
    auto vtr = read (Register32::VTR);

    num_apr = 1U << ((vtr >> 26 & 0x7) + 1) >> 5;
    num_lr  = (vtr & 0x1f) + 1;

    trace (TRACE_INTR, "GICH: %#010llx APR:%u LR:%u", phys, num_apr, num_lr);
}

void Gich::init_regs()
{
    auto vtr = get_el2_vtr();

    num_apr = 1U << ((vtr >> 26 & 0x7) + 1) >> 5;
    num_lr  = static_cast<unsigned>((vtr & 0x1f) + 1);

    trace (TRACE_INTR, "GICH: REGS APR:%u LR:%u", num_apr, num_lr);
}
