/*
 * Application Binary Interface: Architecture-Specific (ARM)
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

#include "regs.hpp"

class Sys_abi
{
    private:
        Sys_regs &s;

    public:
        Sys_abi (Sys_regs &r) : s (r) {}

        auto       &p0() const { return s.gpr[0]; }
        auto       &p1() const { return s.gpr[1]; }
        auto       &p2() const { return s.gpr[2]; }
        auto const &p3() const { return s.gpr[3]; }
        auto const &p4() const { return s.gpr[4]; }

        ALWAYS_INLINE uint8_t flags() const { return p0() >> 4 & BIT_RANGE (3, 0); }
};
