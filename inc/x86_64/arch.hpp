/*
 * Architecture Definitions
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

#ifdef __ASSEMBLER__
#define PREG(X)         %X
#else
#define PREG(X)         %%X
#endif

#define ARCH            "x86_64"
#define BFD_ARCH        "i386:x86-64"
#define BFD_FORMAT      "elf64-x86-64"
#define WORD            .quad
#define SIZE            8
#define ELF_PHDR        Ph64
#define ELF_CLASS       2
#define ELF_MACHINE     62
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
