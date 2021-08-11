/*
 * Architecture Definitions
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "macros.hpp"

#define ARCH                "x86_64"
#define BFD_ARCH            "i386:x86-64"
#define BFD_FORMAT          "elf64-x86-64"
#define ELF_PHDR            Ph64
#define ELF_CLASS           2
#define ELF_MACHINE         62
#define ARG_IP              rcx
#define ARG_SP              r11
#define ARG_1               rdi
#define ARG_2               rsi
#define ARG_3               rdx
#define ARG_4               rax
#define ARG_5               r8

#define EXC_DE              0                       // Divide Error
#define EXC_DB              1                       // Debug Exception
#define EXC_NMI             2                       // Non-Maskable Interrupt
#define EXC_BP              3                       // Breakpoint
#define EXC_OF              4                       // Overflow
#define EXC_BR              5                       // Bound Range Exceeded
#define EXC_UD              6                       // Undefined Opcode
#define EXC_NM              7                       // No Math Coprocessor
#define EXC_DF              8                       // Double Fault
#define EXC_TS              10                      // Invalid TSS
#define EXC_NP              11                      // Segment Not Present
#define EXC_SS              12                      // Stack-Segment Fault
#define EXC_GP              13                      // General Protection Fault
#define EXC_PF              14                      // Page Fault
#define EXC_MF              16                      // Math Fault
#define EXC_AC              17                      // Alignment Check
#define EXC_MC              18                      // Machine Check
#define EXC_XM              19                      // SIMD Floating-Point Exception
#define EXC_VE              20                      // Virtualization Exception
#define EXC_CP              21                      // Control Protection Exception

#define IDT_MASK            3
#define IDT_IST1            2                       // Stack Switching
#define IDT_USER            1                       // User Accessible
