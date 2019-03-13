/*
 * Nested Page Table (NPT)
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

#include "cpu.hpp"
#include "ptab_npt.hpp"

uint64_t Nptp::current;

void Nptp::init()
{
    // Reset at resume time to match vttbr
    current = 0;

    // OAS > 5 requires FEAT_LPA2
    auto const oas { min (Cpu::feature (Cpu::Mem_feature::PARANGE), uint8_t (5)) };

    // IPA cannot be larger than OAS supported by CPU
    assert (Npt::ibits <= Npt::pas (oas));

    asm volatile ("msr vtcr_el2, %x0; isb" : : "rZ" (VTCR_RES1 | oas << 16 | TCR_TG0_4K | TCR_SH0_INNER | TCR_ORGN0_WB_RW | TCR_IRGN0_WB_RW | (Npt::lev() - 2) << 6 | (64 - Npt::ibits)) : "memory");
}
