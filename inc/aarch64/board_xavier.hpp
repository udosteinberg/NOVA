/*
 * Board-Specific Configuration: NVIDIA Xavier
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

#define LOAD_ADDR       0x80000000

#ifndef __ASSEMBLER__

#include "types.hpp"

struct Board
{
    static constexpr uint64 spin_addr = 0;
    static constexpr struct Cpu { uint64 id; } cpu[6] = { { 0x000 }, { 0x001 }, { 0x100 }, { 0x101 }, { 0x200 }, { 0x201 } };
    static constexpr struct Tmr { unsigned ppi, flg; } tmr[2] = { { 10, 0x8 }, { 11, 0x8 } };
    static constexpr struct Gic { uint64 mmio, size; } gic[4] = { { 0x3881000, 0x1000 }, { 0, 0 }, { 0x3882000, 0x2000 }, { 0x3884000, 0x2000 } };
    static constexpr struct Uart { Uart_type type; uint64 mmio; unsigned clock; } uart = { Uart_type::NS16550, 0x3100000, 0 };
    static constexpr struct Smmu { uint64 mmio; struct { unsigned spi, flg; } glb[2], ctx[2]; } smmu[3] =
    {
        { 0x12000000, { { 0xaa, 0x4 }, { 0xab, 0x4 } }, { { 0xaa, 0x4 }, { 0xab, 0x4 } } },     // SMMU0
        { 0x11000000, { { 0xe8, 0x4 }, { 0xe9, 0x4 } }, { { 0xe8, 0x4 }, { 0xe9, 0x4 } } },     // SMMU1
        { 0x10000000, { { 0xf0, 0x4 }, { 0xf1, 0x4 } }, { { 0xf0, 0x4 }, { 0xf1, 0x4 } } },     // SMMU2
    };
};

#endif
