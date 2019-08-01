/*
 * Board-Specific Configuration: QEMU Virtual Board
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

#define RAM_BASE    0x40000000  // 0x40000000 - ...
#define RAM_SIZE    0x10000000  // 256MB

#ifndef __ASSEMBLER__

#include "board_defaults.hpp"

struct Board : Board_defaults
{
    static constexpr Cpu cpu[4] { { 0x0 }, { 0x1 }, { 0x2 }, { 0x3 } };
    static constexpr Tmr tmr[2] { { 0xa, 0x4 }, { 0xb, 0x4 } };
    static constexpr Gic gic[4] { { 0x8000000, 0x10000 }, { 0x80a0000, 0xf60000 }, { 0x8010000, 0x10000 }, { 0x8030000, 0x10000 } };
    static constexpr Uart uart { Debug::Subtype::SERIAL_PL011, 0x9000000, 24'000'000 };
};

#endif
