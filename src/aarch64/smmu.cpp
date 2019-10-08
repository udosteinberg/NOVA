/*
 * System Memory Management Unit
 *
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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

#include "smmu.hpp"
#include "smmu_v2.hpp"

void Smmu::setup()
{
    // SMMU already enumerated by firmware
    if (list) [[likely]]
        return;

    // SMMUv2 enumeration based on board
    for (unsigned i { 0 }; i < sizeof (Board::smmu_v2) / sizeof (*Board::smmu_v2); i++)
        if (Board::smmu_v2[i].mmio)
            new Smmu_v2 (Board::smmu_v2[i]);
}
