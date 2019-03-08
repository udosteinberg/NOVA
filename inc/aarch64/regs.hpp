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
        uintptr_t   r[31]       { 0 };
};

class Exc_regs : public Sys_regs
{
    public:
        struct {
            uint64  sp          { 0 };
            uint64  tpidr       { 0 };
            uint64  tpidrro     { 0 };
        } el0;

        struct {
            uint64  elr         { 0 };
            uint64  spsr        { SPSR_A64_EL0 };
            uint64  esr         { 0 };
            uint64  far         { 0 };
        } el2;

        inline auto &ip()       { return el2.elr; }
        inline auto &sp()       { return el0.sp; }

        inline unsigned emode() const { return el2.spsr & (SPSR_ALL_nRW | SPSR_ALL_M); }

        inline auto ep() const { return el2.esr >> 26; }

        inline void set_ep (uint64 val) { el2.esr = val << 26; }
};
