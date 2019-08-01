/*
 * Board-Specific Configuration: Avnet Ultra96
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

#define LOAD_ADDR       0x7f000000

#ifndef __ASSEMBLER__

#include "types.hpp"

struct Board
{
    static constexpr uint64 spin_addr = 0;
    static constexpr struct Cpu { uint64 id; } cpu[4] = { { 0 }, { 1 }, { 2 }, { 3 } };
    static constexpr struct Tmr { unsigned ppi, flg; } tmr[2] = { { 10, 0x8 }, { 11, 0x8 } };
    static constexpr struct Gic { uint64 mmio, size; } gic[4] = { { 0xf9010000, 0x10000 }, { 0, 0 }, { 0xf9020000, 0x20000 }, { 0xf9040000, 0x20000 } };
    static constexpr struct Uart { Uart_type type; uint64 mmio; unsigned clock; } uart = { Uart_type::CDNS, 0xff010000, 100000000 };
    static constexpr struct Smmu { uint64 mmio; struct { unsigned spi, flg; } glb[1], ctx[1]; } smmu[1] = { { 0xfd800000, { { 0x9b, 0x4 } }, { { 0x9b, 0x4 } } } };
};

#endif
