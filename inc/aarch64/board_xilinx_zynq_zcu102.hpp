/*
 * Board-Specific Configuration: Xilinx Zynq Ultrascale+ MPSoC ZCU102
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

#define RAM_BASE    0x0         // 0x0 - 0x7fffffff

#ifndef __ASSEMBLER__

#include "debug.hpp"

struct Board
{
    static constexpr uint64_t spin_addr { 0 };
    static constexpr struct Cpu { uint64_t id; } cpu[4] { { 0x0 }, { 0x1 }, { 0x2 }, { 0x3 } };
    static constexpr struct Tmr { unsigned ppi, flg; } tmr[2] { { 0xa, 0x8 }, { 0xb, 0x8 } };
    static constexpr struct Gic { uint64_t mmio, size; } gic[4] { { 0xf9010000, 0x10000 }, { 0, 0 }, { 0xf9020000, 0x20000 }, { 0xf9040000, 0x20000 } };
    static constexpr struct Uart { Debug::Subtype type; uint64_t mmio; unsigned clock; } uart { Debug::Subtype::SERIAL_CDNS, 0xff000000, 100'000'000 };
    static constexpr struct Smmu_v2 { uint64_t mmio; struct { unsigned spi, flg; } glb[1], ctx[1]; } smmu_v2[1] { { 0xfd800000, { { 0x9b, 0x4 } }, { { 0x9b, 0x4 } } } };
    static constexpr struct Smmu_v3 { uint64_t mmio; unsigned evt; unsigned glb; unsigned cmd; unsigned pri; } smmu_v3[1] {};
};

#endif
