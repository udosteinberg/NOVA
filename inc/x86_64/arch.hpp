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

#ifdef __ASSEMBLER__
#define PREG(X)         %REG(X)
#define PSEG(X)         %X
#else
#define PREG(X)         %%REG(X)
#define PSEG(X)         %%X
#endif

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

#define LOAD_KSP        mov     PREG(sp), PREG(11);     \
                        mov     tss_run + 4, PREG(sp)

#define SAVE_SEG        sub     $(4 * SIZE), PREG(sp);
#define LOAD_SEG        add     $(6 * SIZE), PREG(sp);

#define SAVE_GPR        push    PREG(ax);               \
                        push    PREG(cx);               \
                        push    PREG(dx);               \
                        push    PREG(bx);               \
                        push    PREG(sp);               \
                        push    PREG(bp);               \
                        push    PREG(si);               \
                        push    PREG(di);               \
                        push    PREG(8);                \
                        push    PREG(9);                \
                        push    PREG(10);               \
                        push    PREG(11);               \
                        push    PREG(12);               \
                        push    PREG(13);               \
                        push    PREG(14);               \
                        push    PREG(15);

#define LOAD_GPR        pop     PREG(15);               \
                        pop     PREG(14);               \
                        pop     PREG(13);               \
                        pop     PREG(12);               \
                        pop     PREG(11);               \
                        pop     PREG(10);               \
                        pop     PREG(9);                \
                        pop     PREG(8);                \
                        pop     PREG(di);               \
                        pop     PREG(si);               \
                        pop     PREG(bp);               \
                        pop     PREG(ax);               \
                        pop     PREG(bx);               \
                        pop     PREG(dx);               \
                        pop     PREG(cx);               \
                        pop     PREG(ax);

#define RET_USER_HYP    mov     PREG(11), PREG(sp);     \
                        mov     $0x200, PREG(11);       \
                        sysretq;

#define RET_USER_EXC    iretq;
