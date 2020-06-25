/*
 * Entry/Exit Functions
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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

#define SAVE_GPR        push    PREG(r15);  \
                        push    PREG(r14);  \
                        push    PREG(r13);  \
                        push    PREG(r12);  \
                        push    PREG(r11);  \
                        push    PREG(r10);  \
                        push    PREG(r9);   \
                        push    PREG(r8);   \
                        push    PREG(rdi);  \
                        push    PREG(rsi);  \
                        push    PREG(rbp);  \
                        push    PREG(rbx);  \
                        push    PREG(rdx);  \
                        push    PREG(rcx);  \
                        push    PREG(rax);

#define LOAD_GPR        pop     PREG(rax);  \
                        pop     PREG(rcx);  \
                        pop     PREG(rdx);  \
                        pop     PREG(rbx);  \
                        pop     PREG(rbp);  \
                        pop     PREG(rsi);  \
                        pop     PREG(rdi);  \
                        pop     PREG(r8);   \
                        pop     PREG(r9);   \
                        pop     PREG(r10);  \
                        pop     PREG(r11);  \
                        pop     PREG(r12);  \
                        pop     PREG(r13);  \
                        pop     PREG(r14);  \
                        pop     PREG(r15);

#define IRET            add     $(__SIZEOF_POINTER__ * 3), PREG(rsp);   \
                        iretq

#define OFS_CR2         (__SIZEOF_POINTER__ * 15)
#define OFS_VEC         (__SIZEOF_POINTER__ * 17)
#define OFS_CS          (__SIZEOF_POINTER__ * 19)
