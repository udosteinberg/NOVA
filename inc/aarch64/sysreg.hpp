/*
 * System Register Access
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

#define RDSYS(T, R, E)                                          \
static inline auto get_##R()                                    \
{                                                               \
    T v;                                                        \
    asm volatile ("mrs %x0, " EXPAND (E) : "=r" (v));           \
    return v;                                                   \
}

#define WRSYS(T, R, E)                                          \
static inline void set_##R (T v)                                \
{                                                               \
    asm volatile ("msr " EXPAND (E) ", %x0" : : "rZ" (v));      \
}

#define SYSREG32_RO(R, E)   RDSYS (uint32_t, R, E)
#define SYSREG32_WO(R, E)   WRSYS (uint32_t, R, E)
#define SYSREG32_RW(R, E)   SYSREG32_RO (R, E) SYSREG32_WO (R, E)

#define SYSREG64_RO(R, E)   RDSYS (uint64_t, R, E)
#define SYSREG64_WO(R, E)   WRSYS (uint64_t, R, E)
#define SYSREG64_RW(R, E)   SYSREG64_RO (R, E) SYSREG64_WO (R, E)
