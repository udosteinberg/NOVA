/*
 * Board-Specific Configuration: Amlogic G12B (S922X/A311D)
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

#define RAM_BASE    0x0         // 0x0 - 0xf4ffffff

#ifndef __ASSEMBLER__

#include "board_defaults.hpp"

struct Board : Board_defaults
{
    static constexpr Cpu cpu[6] { { 0x0 }, { 0x1 }, { 0x100 }, { 0x101 }, { 0x102 }, { 0x103 } };
    static constexpr Tmr tmr[2] { { 0xa, 0x8 }, { 0xb, 0x8 } };
    static constexpr Gic gic[4] { { 0xffc01000, 0x1000 }, { 0, 0 }, { 0xffc02000, 0x2000 }, { 0xffc04000, 0x2000 } };
    static constexpr Uart uart { Debug::Subtype::SERIAL_MESON, 0xff803000, 24'000'000 };
};

#endif
