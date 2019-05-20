/*
 * Register File
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

#include "arch.hpp"
#include "compiler.hpp"
#include "types.hpp"

class Sys_regs
{
    public:
        enum Status
        {
            SUCCESS,
            TIMEOUT,
            ABORTED,
            OVRFLOW,
            BAD_HYP,
            BAD_CAP,
            BAD_PAR,
            BAD_FTR,
            BAD_CPU,
            BAD_DEV,
        };

        mword   r[31]       { 0 };
        mword   el0_sp      { 0 };
        mword   el0_tpidr   { 0 };
        mword   el0_tpidrro { 0 };
        mword   el2_elr     { 0 };
        mword   el2_spsr    { PSTATE_A64_EL0 };

        inline unsigned emode() const { return el2_spsr & (PSTATE_ALL_nRW | PSTATE_ALL_M); }

        inline unsigned flags() const { return r[0] >> 4 & 0xf; }

        inline void set_status (Status status) { r[0] = status; }

        inline void set_p0 (mword val) { r[0]    = val; }
        inline void set_p1 (mword val) { r[1]    = val; }
        inline void set_sp (mword val) { el0_sp  = val; }
        inline void set_ip (mword val) { el2_elr = val; }
};

class Exc_regs : public Sys_regs
{
    public:
        mword   el2_esr     { 0 };
        mword   el2_far     { 0 };

        inline mword ep() const { return el2_esr >> 26; }

        inline void set_ep (mword val) { el2_esr = val << 26; }
};
