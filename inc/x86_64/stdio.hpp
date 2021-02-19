/*
 * Standard I/O
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "console.hpp"
#include "cpu.hpp"
#include "lowlevel.hpp"
#include "macros.hpp"
#include "memory.hpp"

static inline auto stackptr() { return reinterpret_cast<uintptr_t>(__builtin_frame_address (0)); }

#define trace(T,format,...)                         \
do {                                                \
    if ((trace_mask & (T)) == (T)) [[unlikely]]     \
        Console::print ("[%3ld] " format, (stackptr() & ~OFFS_MASK (0)) == MMAP_CPU_DSTB ? ACCESS_ONCE (Cpu::id) : ~0UL, ## __VA_ARGS__);   \
} while (0)

#define panic(format,...)                           \
do {                                                \
    trace (0, "FAIL: " format, ## __VA_ARGS__);     \
    shutdown();                                     \
} while (0)

/*
 * Definition of trace events
 */
enum {
    TRACE_CPU       = BIT  (0),
    TRACE_FPU       = BIT  (1),
    TRACE_PWR       = BIT  (2),
    TRACE_MCA       = BIT  (3),
    TRACE_PCI       = BIT  (4),
    TRACE_TPM       = BIT  (5),
    TRACE_DRTM      = BIT  (6),
    TRACE_INTR      = BIT  (7),
    TRACE_TIMR      = BIT  (8),
    TRACE_SMMU      = BIT  (9),
    TRACE_VIRT      = BIT (10),
    TRACE_FIRM      = BIT (11),
    TRACE_PARSE     = BIT (12),
    TRACE_MEMORY    = BIT (13),
    TRACE_SCHEDULE  = BIT (14),
    TRACE_RCU       = BIT (17),
    TRACE_CREATE    = BIT (18),
    TRACE_DESTROY   = BIT (19),
    TRACE_SYSCALL   = BIT (25),
    TRACE_EXCEPTION = BIT (26),
    TRACE_ROOT      = BIT (27),
    TRACE_PERF      = BIT (28),
    TRACE_CONT      = BIT (29),
    TRACE_KILL      = BIT (30),
    TRACE_ERROR     = BIT (31),
};

/*
 * Enabled trace events
 */
constexpr auto trace_mask
{
    TRACE_CPU       |
    TRACE_PCI       |
    TRACE_TPM       |
    TRACE_DRTM      |
    TRACE_SMMU      |
    TRACE_FIRM      |
    TRACE_ROOT      |
    TRACE_PERF      |
    TRACE_KILL      |
    TRACE_ERROR     |
#ifdef DEBUG
    TRACE_FPU       |
    TRACE_PWR       |
    TRACE_MCA       |
    TRACE_INTR      |
    TRACE_TIMR      |
    TRACE_VIRT      |
#endif
    0
};
