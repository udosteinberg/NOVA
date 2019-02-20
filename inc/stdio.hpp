/*
 * Standard I/O
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "console.hpp"
#include "cpu.hpp"
#include "memory.hpp"

#if defined(__x86_64__)
#define trace(T,format,...)                                         \
do {                                                                \
    register mword __esp asm ("esp");                               \
    if (EXPECT_FALSE ((trace_mask & (T)) == (T)))                   \
        Console::print ("[%2ld] " format,                           \
                static_cast<long>(((__esp - 1) & ~PAGE_MASK) ==     \
                CPU_LOCAL_STCK ? Cpu::id : ~0UL), ## __VA_ARGS__);  \
} while (0)
#elif defined (__aarch64__)
#define trace(T,format,...)                                         \
do {                                                                \
    if (EXPECT_FALSE ((trace_mask & (T)) == (T)))                   \
        Console::print ("[%2ld] " format, ~0UL, ## __VA_ARGS__);    \
} while (0)
#endif

/*
 * Definition of trace events
 */
enum {
    TRACE_CPU       = 1UL << 0,
    TRACE_IOMMU     = 1UL << 1,
    TRACE_APIC      = 1UL << 2,
    TRACE_VMX       = 1UL << 4,
    TRACE_SVM       = 1UL << 5,
    TRACE_FIRM      = 1UL << 7,
    TRACE_ACPI      = 1UL << 8,
    TRACE_INTR      = 1UL << 9,
    TRACE_TIMER     = 1UL << 10,
    TRACE_ROOT      = 1UL << 11,
    TRACE_PTE       = 1UL << 12,
    TRACE_MEMORY    = 1UL << 13,
    TRACE_PCI       = 1UL << 14,
    TRACE_SCHEDULE  = 1UL << 16,
    TRACE_DEL       = 1UL << 18,
    TRACE_REV       = 1UL << 19,
    TRACE_RCU       = 1UL << 20,
    TRACE_FPU       = 1UL << 23,
    TRACE_PERF      = 1UL << 24,
    TRACE_CONT      = 1UL << 25,
    TRACE_PARSE     = 1UL << 26,
    TRACE_CREATE    = 1UL << 27,
    TRACE_SYSCALL   = 1UL << 28,
    TRACE_EXCEPTION = 1UL << 29,
    TRACE_ERROR     = 1UL << 30,
    TRACE_KILL      = 1UL << 31,
};

/*
 * Enabled trace events
 */
unsigned const trace_mask = TRACE_CPU       |
                            TRACE_IOMMU     |
                            TRACE_FIRM      |
                            TRACE_INTR      |
                            TRACE_TIMER     |
                            TRACE_ROOT      |
                            TRACE_PERF      |
                            TRACE_KILL      |
#ifdef DEBUG
                            TRACE_ERROR     |
#endif
                            0;
