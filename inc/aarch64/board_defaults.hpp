/*
 * Board Defaults
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

#pragma once

#include "debug.hpp"

struct Board_defaults
{
    static constexpr uint64_t spin_addr {};
    static constexpr struct Cpu { uint64_t id; } cpu[1] {};
    static constexpr struct Tmr { unsigned ppi, flg; } tmr[2] {};
    static constexpr struct Gic { uint64_t mmio; unsigned size; } gic[4] {};
    static constexpr struct Its { uint64_t mmio; } its[1] {};
    static constexpr struct Smmu_v2 { uint64_t mmio; struct { unsigned spi, flg; } glb[1], ctx[1]; } smmu_v2[1] {};
    static constexpr struct Smmu_v3 { uint64_t mmio; arm_intid_t evt, glb, cmd, pri; } smmu_v3[1] {};
    static constexpr struct Uart { Debug::Subtype type; uint64_t mmio; unsigned clock; } uart { Debug::Subtype::SERIAL_NONE, 0, 0 };
};
