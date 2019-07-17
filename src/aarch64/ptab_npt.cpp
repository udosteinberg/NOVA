/*
 * Nested Page Table (NPT)
 *
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

#include "ptab_npt.hpp"

uint64_t Nptp::current;

void Nptp::init()
{
    // Reset at resume time to match vttbr
    current = 0;

    auto const oas { 2 };

    bool const feat_vmid16 { false };

    // Reduce the number of VMIDs according to what this CPU supports
    Vmid::allocator.reduce (BIT (feat_vmid16 ? 16 : 8));

    asm volatile ("msr vtcr_el2, %x0; isb" : : "r" (VTCR_RES1 | feat_vmid16 * BIT (19) | oas << 16 | TCR_TG0_4K | TCR_SH0_INNER | TCR_ORGN0_WB_RW | TCR_IRGN0_WB_RW | (Npt::lev() - 2) << 6 | (64 - Npt::ibits)) : "memory");
}
