/*
 * Standard I/O
 *
 * Copyright (C) 2006-2009, Udo Steinberg <udo@hypervisor.org>
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
#include "memory.h"
#include "stdarg.h"

#define trace(T,format,...)                                 \
do {                                                        \
    register mword __esp asm ("esp");                       \
    if (EXPECT_FALSE ((trace_mask & T) == T))               \
        printf ("[%d] " format "\n",                        \
               ((__esp - 1) & ~PAGE_MASK) == KSTCK_ADDR ?   \
                Cpu::id : ~0u, ## __VA_ARGS__);             \
} while (0)

/*
 * Definition of trace events
 */
enum {
    TRACE_CPU       = 1ul << 0,
    TRACE_VMX       = 1ul << 1,
    TRACE_APIC      = 1ul << 3,
    TRACE_DMAR      = 1ul << 4,
    TRACE_SVM       = 1ul << 5,
    TRACE_INTERRUPT = 1ul << 7,
    TRACE_ACPI      = 1ul << 8,
    TRACE_KEYB      = 1ul << 9,
    TRACE_ERROR     = 1ul << 10,
    TRACE_SYSCALL   = 1ul << 12,
    TRACE_MEMORY    = 1ul << 13,
    TRACE_PCI       = 1ul << 14,
    TRACE_SCHEDULE  = 1ul << 16,
    TRACE_VTLB      = 1ul << 18,
    TRACE_MAP       = 1ul << 19,
    TRACE_PTE       = 1ul << 20,
    TRACE_EPT       = 1ul << 21,
    TRACE_DPT       = 1ul << 22,
    TRACE_FPU       = 1ul << 23,
};

/*
 * Enabled trace events
 */
unsigned const trace_mask =
                            TRACE_CPU       |
#ifndef NDEBUG
                            TRACE_VMX       |
//                            TRACE_APIC      |
                            TRACE_DMAR      |
                            TRACE_SVM       |
//                            TRACE_INTERRUPT |
//                            TRACE_ACPI      |
//                            TRACE_KEYB      |
                            TRACE_ERROR     |
//                            TRACE_SYSCALL   |
//                            TRACE_MEMORY    |
//                            TRACE_PCI       |
//                            TRACE_SCHEDULE  |
//                            TRACE_VTLB      |
//                            TRACE_MAP       |
//                            TRACE_PTE       |
//                            TRACE_EPT       |
//                            TRACE_DPT       |
//                            TRACE_FPU       |
#endif
                            0;

FORMAT (1,0)
void vprintf (char const *format, va_list args);

FORMAT (1,2)
void printf (char const *format, ...);

FORMAT (1,2) NORETURN
void panic (char const *format, ...);

extern Console_serial serial;
extern Console_vga    screen;
