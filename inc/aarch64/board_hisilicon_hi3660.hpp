/*
 * Board-Specific Configuration: HiSilicon Hi3660
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

#define RAM_BASE    0x0         // 0x0 - 0xdfffffff

#ifndef __ASSEMBLER__

#include "debug.hpp"

struct Board
{
    static constexpr uint64_t spin_addr { 0 };
    static constexpr struct Cpu { uint64_t id; } cpu[8] { { 0x0 }, { 0x1 }, { 0x2 }, { 0x3 }, { 0x100 }, { 0x101 }, { 0x102 }, { 0x103 } };
    static constexpr struct Tmr { unsigned ppi, flg; } tmr[2] { { 0xa, 0x8 }, { 0xb, 0x8 } };
    static constexpr struct Gic { uint64_t mmio, size; } gic[4] { { 0xe82b1000, 0x1000 }, { 0, 0 }, { 0xe82b2000, 0x2000 }, { 0xe82b4000, 0x2000 } };
    static constexpr struct Uart { Debug::Subtype type; uint64_t mmio; unsigned clock; } uart { Debug::Subtype::SERIAL_PL011, 0xfff32000, 0 };
    static constexpr struct Smmu_v2 { uint64_t mmio; struct { unsigned spi, flg; } glb[1], ctx[1]; } smmu_v2[1] {};
    static constexpr struct Smmu_v3 { uint64_t mmio; unsigned evt; unsigned glb; unsigned cmd; unsigned pri; } smmu_v3[1] {};
};

#endif
