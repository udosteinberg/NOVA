/*
 * Standard I/O
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "compiler.h"
#include "console_serial.h"
#include "console_vga.h"
#include "cpu.h"

#define trace(T,format,...)                                 \
do {                                                        \
    register mword __esp asm ("esp");                       \
    if (EXPECT_FALSE ((trace_mask & (T)) == (T)))           \
        printf ("[%2d] " format "\n",                       \
               ((__esp - 1) & ~PAGE_MASK) == KSTCK_ADDR ?   \
                Cpu::id : ~0U, ## __VA_ARGS__);             \
} while (0)

/*
 * Definition of trace events
 */
enum {
    TRACE_CPU       = 1UL << 0,
    TRACE_IOMMU     = 1UL << 1,
    TRACE_APIC      = 1UL << 2,
    TRACE_KEYB      = 1UL << 3,
    TRACE_VMX       = 1UL << 4,
    TRACE_SVM       = 1UL << 5,
    TRACE_ACPI      = 1UL << 8,
    TRACE_MEMORY    = 1UL << 13,
    TRACE_PCI       = 1UL << 14,
    TRACE_SCHEDULE  = 1UL << 16,
    TRACE_VTLB      = 1UL << 17,
    TRACE_DEL       = 1UL << 18,
    TRACE_REV       = 1UL << 19,
    TRACE_RCU       = 1UL << 20,
    TRACE_FPU       = 1UL << 23,
    TRACE_SYSCALL   = 1UL << 30,
    TRACE_ERROR     = 1UL << 31,
};

/*
 * Enabled trace events
 */
unsigned const trace_mask =
                            TRACE_CPU       |
                            TRACE_IOMMU     |
#ifdef DEBUG
//                            TRACE_APIC      |
//                            TRACE_KEYB      |
                            TRACE_VMX       |
                            TRACE_SVM       |
//                            TRACE_ACPI      |
//                            TRACE_MEMORY    |
//                            TRACE_PCI       |
//                            TRACE_SCHEDULE  |
//                            TRACE_VTLB      |
//                            TRACE_DEL       |
//                            TRACE_REV       |
//                            TRACE_RCU       |
//                            TRACE_FPU       |
//                            TRACE_SYSCALL   |
                            TRACE_ERROR     |
#endif
                            0;

FORMAT (1,2)
void printf (char const *format, ...);

FORMAT (1,2) NORETURN
void panic (char const *format, ...);

extern Console_serial serial;
extern Console_vga    screen;
