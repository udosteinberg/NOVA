/*
 * Board-Specific Configuration: Xilinx Zynq Ultrascale+ MPSoC ZCU102 (ZUxCG)
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

#include "board_defaults.hpp"

struct Board : Board_defaults
{
    static constexpr Cpu cpu[2] { { 0x0 }, { 0x1 } };
    static constexpr Tmr tmr[2] { { 0xa, 0x8 }, { 0xb, 0x8 } };
    static constexpr Gic gic[4] { { 0xf9010000, 0x10000 }, { 0, 0 }, { 0xf9020000, 0x20000 }, { 0xf9040000, 0x20000 } };
    static constexpr Uart uart { Debug::Subtype::SERIAL_CDNS, 0xff000000, 100'000'000 };

    static constexpr struct Smmu_v2 { uint64_t mmio; struct { unsigned spi, flg; } glb[1], ctx[1]; } smmu_v2[1] { { 0xfd800000, { { 0x9b, 0x4 } }, { { 0x9b, 0x4 } } } };
};

#endif
