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

#define EXC_DB              1
#define EXC_NM              7
#define EXC_TS              10
#define EXC_GP              13
#define EXC_PF              14
#define EXC_AC              17
#define EXC_MC              18

#define EFL_CF              BIT64  (0)              // Carry Flag
#define EFL_PF              BIT64  (2)              // Parity Flag
#define EFL_AF              BIT64  (4)              // Auxiliary Carry Flag
#define EFL_ZF              BIT64  (6)              // Zero Flag
#define EFL_SF              BIT64  (7)              // Sign Flag
#define EFL_TF              BIT64  (8)              // Trap Flag
#define EFL_IF              BIT64  (9)              // Interrupt Enable Flag
#define EFL_DF              BIT64 (10)              // Direction Flag
#define EFL_OF              BIT64 (11)              // Overflow Flag
#define EFL_IOPL            BIT64_RANGE (13, 12)    // I/O Privilege Level
#define EFL_NT              BIT64 (14)              // Nested Task Flag
#define EFL_RF              BIT64 (16)              // Resume Flag
#define EFL_VM              BIT64 (17)              // Virtual-8086 Mode Flag
#define EFL_AC              BIT64 (18)              // Alignment Check Flag
#define EFL_VIF             BIT64 (19)              // Virtual Interrupt Flag
#define EFL_VIP             BIT64 (20)              // Virtual Interrupt Pending Flag
#define EFL_ID              BIT64 (21)              // Identification Flag

#define CR0_PE              BIT64  (0)              // Protection Enable
#define CR0_MP              BIT64  (1)              // Monitor Coprocessor
#define CR0_EM              BIT64  (2)              // Emulation
#define CR0_TS              BIT64  (3)              // Task Switched
#define CR0_ET              BIT64  (4)              // Extension Type
#define CR0_NE              BIT64  (5)              // Numeric Error
#define CR0_WP              BIT64 (16)              // Write Protect
#define CR0_AM              BIT64 (18)              // Alignment Mask
#define CR0_NW              BIT64 (29)              // Not Write-Through
#define CR0_CD              BIT64 (30)              // Cache Disable
#define CR0_PG              BIT64 (31)              // Paging

#define CR4_VME             BIT64  (0)
#define CR4_PVI             BIT64  (1)
#define CR4_TSD             BIT64  (2)
#define CR4_DE              BIT64  (3)
#define CR4_PSE             BIT64  (4)
#define CR4_PAE             BIT64  (5)
#define CR4_MCE             BIT64  (6)
#define CR4_PGE             BIT64  (7)
#define CR4_PCE             BIT64  (8)
#define CR4_OSFXSR          BIT64  (9)
#define CR4_OSXMMEXCPT      BIT64 (10)
#define CR4_UMIP            BIT64 (11)
#define CR4_LA57            BIT64 (12)
#define CR4_VMXE            BIT64 (13)
#define CR4_SMXE            BIT64 (14)
#define CR4_FSGSBASE        BIT64 (16)
#define CR4_PCIDE           BIT64 (17)
#define CR4_OSXSAVE         BIT64 (18)
#define CR4_SMEP            BIT64 (20)
#define CR4_SMAP            BIT64 (21)
#define CR4_PKE             BIT64 (22)
#define CR4_CET             BIT64 (23)
#define CR4_PKS             BIT64 (24)

#define EFER_SCE            BIT64  (0)
#define EFER_LME            BIT64  (8)
#define EFER_LMA            BIT64 (10)
#define EFER_NXE            BIT64 (11)
#define EFER_SVME           BIT64 (12)
