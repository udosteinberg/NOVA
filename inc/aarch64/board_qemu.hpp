/*
 * Board-Specific Configuration: QEMU Virtual Board
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

#pragma once

#define LOAD_ADDR       0x48000000

#ifndef __ASSEMBLER__

#include "types.hpp"

struct Board
{
    static constexpr uint64 spin_addr { 0 };
    static constexpr struct Cpu { uint64 id; } cpu[4] { { 0x0 }, { 0x1 }, { 0x2 }, { 0x3 } };
    static constexpr struct Tmr { unsigned ppi, flg; } tmr[2] { { 0xa, 0x4 }, { 0xb, 0x4 } };
    static constexpr struct Gic { uint64 mmio, size; } gic[4] { { 0x8000000, 0x10000 }, { 0x80a0000, 0xf60000 }, { 0x8010000, 0x10000 }, { 0x8030000, 0x10000 } };
    static constexpr struct Uart { Uart_type type; uint64 mmio; unsigned clock; } uart { Uart_type::PL011, 0x9000000, 24'000'000 };
    static constexpr struct Smmu { uint64 mmio; struct { unsigned spi, flg; } glb[1], ctx[1]; } smmu[1] { { 0, { { 0, 0 } }, { { 0, 0 } } } };
};

#endif
