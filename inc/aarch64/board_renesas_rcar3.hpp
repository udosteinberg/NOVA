/*
 * Board-Specific Configuration: Renesas R-Car
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

#define RAM_BASE    0x40000000  // 0x40000000 - 0xbfffffff

#ifndef __ASSEMBLER__

#include "board_defaults.hpp"

struct Board : Board_defaults
{
    static constexpr Cpu cpu[6] { { 0x0 }, { 0x1 }, { 0x101 }, { 0x102 }, { 0x103 }, { 0x104 } };
    static constexpr Tmr tmr[2] { { 0xa, 0x8 }, { 0xb, 0x8 } };
    static constexpr Gic gic[4] { { 0xf1010000, 0x1000 }, { 0, 0 }, { 0xf1020000, 0x20000 }, { 0xf1040000, 0x20000 } };
    static constexpr Uart uart { Debug::Subtype::SERIAL_SCIF, 0xe6e88000, 0 };
};

#endif
