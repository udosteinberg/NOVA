/*
 * Generic Interrupt Controller: Hypervisor Interface (GICH/ICH)
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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
#include "gich.hpp"
#include "ptab_hpt.hpp"
#include "space_hst.hpp"
#include "stdio.hpp"

unsigned Gich::num_apr  { 0 };
unsigned Gich::num_lr   { 0 };

void Gich::init()
{
    if (!Acpi::resume && Cpu::bsp && Gicc::mode == Gicc::Mode::MMIO)
        mmap_mmio();

    switch (Gicc::mode) {
        case Gicc::Mode::MMIO: return init_mmio();
        case Gicc::Mode::REGS: return init_regs();
    }
}

void Gich::mmap_mmio()
{
    if (EXPECT_FALSE (!phys))
        panic ("%s failure", __PRETTY_FUNCTION__);

    // FIXME
    constexpr auto size { Board::gic[3].size };

    // Map MMIO region
    Hptp::master_map (MMAP_GLB_GICH, phys, 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev());

    // Reserve MMIO region
    Space_hst::user_access (phys, size, false);

}

void Gich::init_mmio()
{
    auto const vtr { read (Reg32::VTR) };

    num_apr = BIT ((vtr >> 26 & BIT_RANGE (2, 0)) + 1) >> 5;
    num_lr  = (vtr & BIT_RANGE (5, 0)) + 1;

    trace (TRACE_INTR, "GICH: %#010lx APR:%u LR:%u", phys, num_apr, num_lr);
}

void Gich::init_regs()
{
    auto const vtr { get_el2_vtr() };

    num_apr = BIT ((vtr >> 26 & BIT_RANGE (2, 0)) + 1) >> 5;
    num_lr  = (vtr & BIT_RANGE (4, 0)) + 1;

    trace (TRACE_INTR, "GICH: REGS APR:%u LR:%u", num_apr, num_lr);
}
