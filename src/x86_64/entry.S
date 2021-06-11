/*
 * Entry Functions
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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
 *
 */

#include "entry.hpp"
#include "vectors.hpp"

.data
.global                 handlers
handlers:

.text

.macro                  INTRGATE DPL
                        .align  4, 0x90
1:
                        .data
                        .quad   1b + \DPL
                        .previous
.endm

.macro                  TASKGATE
                        .data
                        .quad   0
                        .previous
.endm

.macro                  EXCEPTION_ERROR VEC, DPL
INTRGATE                \DPL
                        push    (%rsp)
                        movq    $\VEC, __SIZEOF_POINTER__(%rsp)
                        jmp     entry_exc_error
.endm

.macro                  EXCEPTION VEC, DPL
INTRGATE                \DPL
                        push    $\VEC
                        jmp     entry_exc
.endm

.macro                  INTERRUPT VEC, LABEL
INTRGATE                0
                        push    $\VEC
                        jmp     \LABEL
.endm

.macro                  SAVE_STATE N
.rept                   \N
                        push    $0
.endr
                        SAVE_GPR
                        mov     %rsp, %rbx
                        testb   $3, OFS_CS(%rbx)
                        jz      1f
                        cld
                        lea     DSTK_TOP, %rsp
1:
.endm

/*
 * Exceptions
 */
EXCEPTION               0x0,    0
EXCEPTION               0x1,    0
TASKGATE
EXCEPTION               0x3,    3
EXCEPTION               0x4,    3
EXCEPTION               0x5,    0
EXCEPTION               0x6,    0
EXCEPTION               0x7,    0
TASKGATE
TASKGATE
EXCEPTION_ERROR         0xa,    0
EXCEPTION_ERROR         0xb,    0
EXCEPTION_ERROR         0xc,    0
EXCEPTION_ERROR         0xd,    0
EXCEPTION_ERROR         0xe,    0
TASKGATE
EXCEPTION               0x10,   0
EXCEPTION_ERROR         0x11,   0
EXCEPTION               0x12,   0
EXCEPTION               0x13,   0
.rept                   NUM_EXC - 0x14
TASKGATE
.endr

/*
 * Exception Entry
 */
entry_exc:              push    $0
entry_exc_error:        SAVE_STATE 1
                        mov     %cr2, %rax
                        mov     %rax, OFS_CR2(%rbx)
                        mov     %rbx, %rdi
                        call    exc_handler
exit:                   testb   $3, OFS_CS(%rbx)
                        jnz     ret_user_iret
                        LOAD_GPR
                        IRET

/*
 * Interrupt Entry
 */
.macro                  INTERRUPT_ENTRY FUNC
                        SAVE_STATE 2
                        mov     OFS_VEC(%rbx), %rdi
                        call    \FUNC
                        jmp     exit
.endm

/*
 * Interrupts
 */
.set                    VEC,    NUM_EXC

entry_gsi:              INTERRUPT_ENTRY gsi_vector
.rept                   NUM_GSI
INTERRUPT               VEC,    entry_gsi
.set                    VEC,    VEC + 1
.endr

entry_msi:              INTERRUPT_ENTRY msi_vector
.rept                   NUM_MSI
INTERRUPT               VEC,    entry_msi
.set                    VEC,    VEC + 1
.endr

entry_lvt:              INTERRUPT_ENTRY lvt_vector
.rept                   NUM_LVT
INTERRUPT               VEC,    entry_lvt
.set                    VEC,    VEC + 1
.endr

entry_ipi:              INTERRUPT_ENTRY ipi_vector
.rept                   NUM_IPI
INTERRUPT               VEC,    entry_ipi
.set                    VEC,    VEC + 1
.endr

/*
 * System-Call Entry
 */
.align                  4, 0x90
.globl                  entry_sys
entry_sys:              mov     %rsp, %r11
                        mov     tss_run + 4, %rsp
                        lea     -(__SIZEOF_POINTER__ * 8)(%rsp), %rsp
                        SAVE_GPR
                        lea     DSTK_TOP, %rsp
                        and     $0xf, %rdi
                        jmp     *syscall(, %rdi, __SIZEOF_POINTER__)

/*
 * VMX Entry
 */
.align                  4, 0x90
.globl                  entry_vmx
entry_vmx:              SAVE_GPR
                        mov     %cr2, %rax
                        mov     %rax, OFS_CR2(%rsp)
                        lea     DSTK_TOP, %rsp
                        jmp     vmx_handler
