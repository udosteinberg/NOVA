/*
 * Board-Specific Configuration: NXP i.MX 8M Quad
 *
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#define LOAD_ADDR       0xff000000

#ifndef __ASSEMBLER__

#include "types.hpp"

struct Board
{
    static constexpr uint64 spin_addr = 0;
    static constexpr struct Cpu { uint64 id; } cpu[4] = { { 0 }, { 1 }, { 2 }, { 3 } };
    static constexpr struct Tmr { unsigned ppi, flg; } tmr[2] = { { 10, 0x8 }, { 11, 0x8 } };
    static constexpr struct Gic { uint64 mmio, size; } gic[4] = { { 0x38800000, 0x10000 }, { 0x38880000, 0xc0000 }, { 0, 0 }, { 0, 0 } };
    static constexpr struct Uart { Uart_type type; uint64 mmio; unsigned clock; } uart = { Uart_type::IMX, 0x30860000, 25000000 };
    static constexpr struct Smmu { uint64 mmio; struct { unsigned spi, flg; } glb[1], ctx[1]; } smmu[1] = { { 0, { { 0, 0 } }, { { 0, 0 } } } };
};

#endif
