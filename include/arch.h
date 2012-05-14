/*
 * Architecture Definitions
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#ifdef __i386__
#define ARCH            "x86_32"
#define WORD            .long
#define SIZE            4
#define ELF_PHDR        Ph32
#define ELF_CLASS       1
#define ELF_MACHINE     3
#define PTE_BPL         10
#define PTE_LEV         2
#define REG(X)          e##X
#define ARG_IP          REG(dx)
#define ARG_SP          REG(cx)
#define ARG_1           REG(ax)
#define ARG_2           REG(di)
#define ARG_3           REG(si)
#define ARG_4           REG(bx)
#define ARG_5           REG(bp)
#define OFS_CR2         0xc
#define OFS_VEC         0x34
#define OFS_CS          0x3c

#ifdef __ASSEMBLER__
.macro  LOAD_KSP
                        pop     %esp
1:                      cld
.endm
.macro  SAVE_SEG
                        push    %ds
                        push    %es
                        push    %fs
                        push    %gs
.endm
.macro  SAVE_GPR
                        pusha
.endm
#endif

#define RESTORE_GPR     "popa;"
#define RESTORE_SEG     "pop %%gs; pop %%fs; pop %%es; pop %%ds; add $8, %%esp;"
#define RET_USER_HYP    "sti; sysexit;"
#define RET_USER_EXC    "iret;"
#endif

#ifdef __x86_64__
#define ARCH            "x86_64"
#define WORD            .quad
#define SIZE            8
#define ELF_PHDR        Ph64
#define ELF_CLASS       2
#define ELF_MACHINE     62
#define PTE_BPL         9
#define PTE_LEV         4
#define REG(X)          r##X
#define ARG_IP          REG(cx)
#define ARG_SP          REG(11)
#define ARG_1           REG(di)
#define ARG_2           REG(si)
#define ARG_3           REG(dx)
#define ARG_4           REG(ax)
#define ARG_5           REG(8)
#define OFS_CR2         0x58
#define OFS_VEC         0xa8
#define OFS_CS          0xb8

#ifdef __ASSEMBLER__
.macro  LOAD_KSP
                        mov     %REG(sp), %REG(11)
                        mov     tss_run + 4, %REG(sp)
.endm
.macro  SAVE_SEG
                        lea -4*SIZE(%REG(sp)), %REG(sp)
.endm
.macro  SAVE_GPR
                        push    %rax
                        push    %rcx
                        push    %rdx
                        push    %rbx
                        push    %rsp
                        push    %rbp
                        push    %rsi
                        push    %rdi
                        push    %r8
                        push    %r9
                        push    %r10
                        push    %r11
                        push    %r12
                        push    %r13
                        push    %r14
                        push    %r15
.endm
#endif

#define RESTORE_GPR     "pop %%r15; pop %%r14; pop %%r13; pop %%r12;"   \
                        "pop %%r11; pop %%r10; pop %%r9;  pop %%r8;"    \
                        "pop %%rdi; pop %%rsi; pop %%rbp; pop %%rax;"   \
                        "pop %%rbx; pop %%rdx; pop %%rcx; pop %%rax;"
#define RESTORE_SEG     "add $48, %%rsp;"
#define RET_USER_HYP    "mov %%r11, %%rsp; mov $0x200, %%r11; sysretq;"
#define RET_USER_EXC    "iretq;"
#endif
