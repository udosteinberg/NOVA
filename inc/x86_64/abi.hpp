/*
 * Application Binary Interface: Architecture-Specific (x86)
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

#include "regs.hpp"

class Sys_abi
{
    private:
        Sys_regs &s;

    public:
        inline Sys_abi (Sys_regs &r) : s (r) {}

        inline auto       &p0() const { return s.rdi; }
        inline auto       &p1() const { return s.rsi; }
        inline auto       &p2() const { return s.rdx; }
        inline auto const &p3() const { return s.rax; }
        inline auto const &p4() const { return s.r8;  }

        ALWAYS_INLINE inline uint8 flags() const { return p0() >> 4 & BIT_RANGE (3, 0); }
};
