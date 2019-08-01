/*
 * Board-Specific Configuration: Texas Instruments Jacinto J721E (DRA829/TDA4VM)
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

#define RAM_BASE    0x80000000  // 0x80000000 - 0xffffffff

#ifndef __ASSEMBLER__

#include "board_defaults.hpp"

struct Board : Board_defaults
{
    static constexpr Cpu cpu[2] { { 0x0 }, { 0x1 } };
    static constexpr Tmr tmr[2] { { 0xa, 0x8 }, { 0xb, 0x8 } };
    static constexpr Gic gic[4] { { 0x1800000, 0x10000 }, { 0x1900000, 0x100000 }, { 0, 0 }, { 0, 0 } };
    static constexpr Its its[1] { { 0x1820000 } };
    static constexpr Uart uart { Debug::Subtype::SERIAL_NS16550_DBGP, 0x2800000, 48'000'000 };
    static constexpr Smmu_v3 smmu_v3[1] { 0x36600000, 0x324, 0x320, 0, 0 };
};

#endif
