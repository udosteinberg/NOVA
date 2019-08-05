/*
 * Generic Interrupt Controller: Hypervisor Interface (GICH/ICH)
 *
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
#include "gich.hpp"
#include "ptab_hpt.hpp"
#include "space_hst.hpp"
#include "stdio.hpp"

unsigned Gich::num_apr  { 0 };
unsigned Gich::num_lr   { 0 };

void Gich::init()
{
    if (!Acpi::resume && Cpu::bsp && !mmap_mmio() && Gicc::mode == Gicc::Mode::MMIO)
        Console::panic ("GICH: MMIO unavailable!");

    switch (Gicc::mode) {
        case Gicc::Mode::MMIO: init_mmio(); break;
        case Gicc::Mode::REGS: init_regs(); break;
    }
}

bool Gich::mmap_mmio()
{
    if (!phys)
        return false;

    // FIXME
    constexpr auto size { Board::gic[3].size };

    // Reserve MMIO region
    Space_hst::user_access (phys, size, false);

    // Map MMIO region if needed
    if (Gicc::mode == Gicc::Mode::MMIO)
        Hptp::master_map (MMAP_GLB_GICH, phys, 0,
                          Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev());

    return true;
}

void Gich::init_mmio()
{
    auto const vtr { read (Register32::VTR) };

    num_apr = BIT ((vtr >> 26 & BIT_RANGE (2, 0)) + 1) >> 5;
    num_lr  = (vtr & BIT_RANGE (5, 0)) + 1;

    trace (TRACE_INTR, "GICH: %#010llx APR:%u LR:%u", phys, num_apr, num_lr);
}

void Gich::init_regs()
{
    auto const vtr { get_el2_vtr() };

    num_apr = BIT ((vtr >> 26 & BIT_RANGE (2, 0)) + 1) >> 5;
    num_lr  = (vtr & BIT_RANGE (4, 0)) + 1;

    trace (TRACE_INTR, "GICH: REGS APR:%u LR:%u", num_apr, num_lr);
}
