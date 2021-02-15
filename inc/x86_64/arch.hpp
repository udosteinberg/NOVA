/*
 * Architecture Definitions: x86
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "macros.hpp"

#ifdef __ASSEMBLER__
#define PREG(X)         %X
#else
#define PREG(X)         %%X
#endif

#define ARCH            "x86_64"
#define WORD            .quad
#define SIZE            8
#define ELF_PHDR        Ph64
#define ELF_CLASS       EC_64
#define ELF_MACHINE     EM_X86_64
#define PTE_BPL         9
#define PTE_LEV         4
#define ARG_IP          rcx
#define ARG_SP          r11
#define ARG_1           rdi
#define ARG_2           rsi
#define ARG_3           rdx
#define ARG_4           rax
#define ARG_5           r8
#define OFS_CR2         0x58
#define OFS_VEC         0xa8
#define OFS_CS          0xb8

#define SAVE_SEG        sub     $(4 * SIZE), PREG(rsp);
#define LOAD_SEG        add     $(6 * SIZE), PREG(rsp);

#define SAVE_GPR        push    PREG(rax);              \
                        push    PREG(rcx);              \
                        push    PREG(rdx);              \
                        push    PREG(rbx);              \
                        push    PREG(rsp);              \
                        push    PREG(rbp);              \
                        push    PREG(rsi);              \
                        push    PREG(rdi);              \
                        push    PREG(r8);               \
                        push    PREG(r9);               \
                        push    PREG(r10);              \
                        push    PREG(r11);              \
                        push    PREG(r12);              \
                        push    PREG(r13);              \
                        push    PREG(r14);              \
                        push    PREG(r15);

#define LOAD_GPR        pop     PREG(r15);              \
                        pop     PREG(r14);              \
                        pop     PREG(r13);              \
                        pop     PREG(r12);              \
                        pop     PREG(r11);              \
                        pop     PREG(r10);              \
                        pop     PREG(r9);               \
                        pop     PREG(r8);               \
                        pop     PREG(rdi);              \
                        pop     PREG(rsi);              \
                        pop     PREG(rbp);              \
                        pop     PREG(rax);              \
                        pop     PREG(rbx);              \
                        pop     PREG(rdx);              \
                        pop     PREG(rcx);              \
                        pop     PREG(rax);

#define RET_USER_HYP    mov     PREG(r11), PREG(rsp);   \
                        mov     $0x200, PREG(r11);      \
                        sysretq;

#define RET_USER_EXC    iretq;

#define EXC_DE               0                      // Divide Error
#define EXC_DB               1                      // Debug Exception
#define EXC_BP               3                      // Breakpoint
#define EXC_OF               4                      // Overflow
#define EXC_BR               5                      // Bound Range Exceeded
#define EXC_UD               6                      // Undefined Opcode
#define EXC_NM               7                      // No Math Coprocessor
#define EXC_DF               8                      // Double Fault
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

#define RFL_ID              BIT64 (21)              // Identification Flag
#define RFL_VIP             BIT64 (20)              // Virtual Interrupt Pending Flag
#define RFL_VIF             BIT64 (19)              // Virtual Interrupt Flag
#define RFL_AC              BIT64 (18)              // Alignment Check Flag
#define RFL_VM              BIT64 (17)              // Virtual-8086 Mode Flag
#define RFL_RF              BIT64 (16)              // Resume Flag
#define RFL_NT              BIT64 (14)              // Nested Task Flag
#define RFL_IOPL            BIT64_RANGE (13, 12)    // I/O Privilege Level
#define RFL_OF              BIT64 (11)              // Overflow Flag
#define RFL_DF              BIT64 (10)              // Direction Flag
#define RFL_IF              BIT64  (9)              // Interrupt Enable Flag
#define RFL_TF              BIT64  (8)              // Trap Flag
#define RFL_SF              BIT64  (7)              // Sign Flag
#define RFL_ZF              BIT64  (6)              // Zero Flag
#define RFL_AF              BIT64  (4)              // Auxiliary Carry Flag
#define RFL_PF              BIT64  (2)              // Parity Flag
#define RFL_CF              BIT64  (0)              // Carry Flag

#define CR0_PG              BIT64 (31)              // Paging
#define CR0_CD              BIT64 (30)              // Cache Disable
#define CR0_NW              BIT64 (29)              // Not Write-Through
#define CR0_AM              BIT64 (18)              // Alignment Mask
#define CR0_WP              BIT64 (16)              // Write Protect
#define CR0_NE              BIT64  (5)              // Numeric Error
#define CR0_ET              BIT64  (4)              // Extension Type
#define CR0_TS              BIT64  (3)              // Task Switched
#define CR0_EM              BIT64  (2)              // Emulation
#define CR0_MP              BIT64  (1)              // Monitor Coprocessor
#define CR0_PE              BIT64  (0)              // Protection Enable

#define CR3_LAM_U48         BIT64 (62)
#define CR3_LAM_U57         BIT64 (61)

#define CR4_FRED            BIT64 (32)              // FRED Transitions Enable
#define CR4_LAM_SUP         BIT64 (28)              // Linear Address Masking for Supervisor Pointers
#define CR4_UINTR           BIT64 (25)              // User Interrupts Enable
#define CR4_PKS             BIT64 (24)              // Protection Keys for Supervisor Pages Enable
#define CR4_CET             BIT64 (23)              // Control-Flow Enforcement Technology
#define CR4_PKE             BIT64 (22)              // Protection Keys for User Pages Enable
#define CR4_SMAP            BIT64 (21)              // SMAP Enable
#define CR4_SMEP            BIT64 (20)              // SMEP Enable
#define CR4_KL              BIT64 (19)              // Key Locker Enable
#define CR4_OSXSAVE         BIT64 (18)              // XSAVE and Processor Extended States Enable
#define CR4_PCIDE           BIT64 (17)              // PCID Enable
#define CR4_FSGSBASE        BIT64 (16)              // FSGSBASE Enable
#define CR4_SEE             BIT64 (15)              // SGX Enable
#define CR4_SMXE            BIT64 (14)              // SMX Enable
#define CR4_VMXE            BIT64 (13)              // VMX Enable
#define CR4_LA57            BIT64 (12)              // 57-Bit Linear Addresses
#define CR4_UMIP            BIT64 (11)              // User-Mode Instruction Prevention
#define CR4_OSXMMEXCPT      BIT64 (10)              // OS Support for Unmasked SIMD Floating-Point Exceptions
#define CR4_OSFXSR          BIT64  (9)              // OS Support for FXSAVE/FXRSTOR
#define CR4_PCE             BIT64  (8)              // Performance-Monitoring Counter Enable
#define CR4_PGE             BIT64  (7)              // Page Global Enable
#define CR4_MCE             BIT64  (6)              // Machine-Check Enable
#define CR4_PAE             BIT64  (5)              // Physical Address Extension
#define CR4_PSE             BIT64  (4)              // Page Size Extensions
#define CR4_DE              BIT64  (3)              // Debugging Extensions
#define CR4_TSD             BIT64  (2)              // Time Stamp Disable
#define CR4_PVI             BIT64  (1)              // Protected-Mode Virtual Interrupts
#define CR4_VME             BIT64  (0)              // Virtual-8086 Mode Extensions

#define EFER_SVME           BIT64 (12)              // SVM Enable
#define EFER_NXE            BIT64 (11)              // Non-Execute Enable
#define EFER_LMA            BIT64 (10)              // Long Mode Active
#define EFER_LME            BIT64  (8)              // Long Mode Enable
#define EFER_SCE            BIT64  (0)              // SYSCALL Enable
