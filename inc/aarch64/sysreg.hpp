/*
 * System Register Access
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

#include "compiler.hpp"
#include "types.hpp"

#define DEFINE_SYSREG_RO(REG, ENC)                              \
ALWAYS_INLINE                                                   \
static inline auto get_##REG()                                  \
{                                                               \
    uint64 val;                                                 \
    asm volatile ("mrs %0, " EXPAND (ENC) : "=r" (val));        \
    return val;                                                 \
}

#define DEFINE_SYSREG_WO(REG, ENC)                              \
ALWAYS_INLINE                                                   \
static inline void set_##REG (uint64 val)                       \
{                                                               \
    asm volatile ("msr " EXPAND (ENC) ", %0" : : "r" (val));    \
}

#define DEFINE_SYSREG_RW(REG, ENC)                              \
        DEFINE_SYSREG_RO(REG, ENC)                              \
        DEFINE_SYSREG_WO(REG, ENC)
