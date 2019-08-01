/*
 * Board-Specific Configuration: NXP i.MX 8M
 *
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

#define RAM_BASE    0x40000000  // 0x40000000 - 0x13fffffff

#ifndef __ASSEMBLER__

#include "board_defaults.hpp"

struct Board : Board_defaults
{
    static constexpr Cpu cpu[4] { { 0x0 }, { 0x1 }, { 0x2 }, { 0x3 } };
    static constexpr Tmr tmr[2] { { 0xa, 0x8 }, { 0xb, 0x8 } };
    static constexpr Gic gic[4] { { 0x38800000, 0x10000 }, { 0x38880000, 0x80000 }, { 0, 0 }, { 0, 0 } };
    static constexpr Uart uart { Debug::Subtype::SERIAL_IMX, 0x30860000, 25'000'000 };
};

#endif
