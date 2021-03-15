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

#include "arch.hpp"
#include "entry.hpp"
#include "vectors.hpp"

.data
.global                 handlers
handlers:

.text

.macro                  INTRGATE DPL
                        .align  4, 0x90
1:                      .data
                        .quad   1b + \DPL
                        .previous
.endm

.macro                  TASKGATE
                        .data
                        .quad   0
                        .previous
.endm

.macro                  SAVE_STATE N
.rept                   \N
                        push    $0
.endr
                        SAVE_GPR
                        mov     %rsp, %rbx
                        testb   $3, OFS_CS (%rbx)
                        jz      1f
                        cld
                        lea     STCK_TOP, %rsp
1:
.endm

.macro                  EXCEPTION VEC, DPL
INTRGATE                \DPL
                        push    $\VEC
                        jmp     entry_exc
.endm

.macro                  EXCEPTION_ERROR VEC, DPL
INTRGATE                \DPL
                        push    (%rsp)
                        movq    $\VEC, __SIZEOF_POINTER__(%rsp)
                        jmp     entry_exc_error
.endm

/*
 * Exception Entries
 */
entry_exc:              push    $0
entry_exc_error:        SAVE_STATE 1
                        mov     %cr2, %rax
                        mov     %rax, OFS_CR2 (%rbx)
                        mov     %rbx, %rdi
                        call    exc_handler
                        jmp     ret_from_interrupt

EXCEPTION               EXC_DE, 0
EXCEPTION               EXC_DB, 0
TASKGATE
EXCEPTION               EXC_BP, 3
EXCEPTION               EXC_OF, 3
EXCEPTION               EXC_BR, 0
EXCEPTION               EXC_UD, 0
EXCEPTION               EXC_NM, 0
TASKGATE
TASKGATE
EXCEPTION_ERROR         EXC_TS, 0
EXCEPTION_ERROR         EXC_NP, 0
EXCEPTION_ERROR         EXC_SS, 0
EXCEPTION_ERROR         EXC_GP, 0
EXCEPTION_ERROR         EXC_PF, 0
TASKGATE
EXCEPTION               EXC_MF, 0
EXCEPTION_ERROR         EXC_AC, 0
EXCEPTION               EXC_MC, 0
EXCEPTION               EXC_XM, 0
EXCEPTION               EXC_VE, 0
EXCEPTION_ERROR         EXC_CP, 0
.rept                   NUM_EXC - 0x16
TASKGATE
.endr

/*
 * GSI Entries
 */
.set                    VEC, NUM_EXC
.rept                   NUM_GSI
INTRGATE                0
                        push    $VEC
                        jmp     entry_gsi
.set                    VEC, VEC + 1
.endr

entry_gsi:              SAVE_STATE 2
                        mov     OFS_VEC (%rbx), %rdi
                        call    gsi_vector
                        jmp     ret_from_interrupt

/*
 * LVT Entries
 */
.rept                   NUM_LVT
INTRGATE                0
                        push    $VEC
                        jmp     entry_lvt
.set                    VEC, VEC + 1
.endr

entry_lvt:              SAVE_STATE 2
                        mov     OFS_VEC (%rbx), %rdi
                        call    lvt_vector
                        jmp     ret_from_interrupt

/*
 * MSI Entries
 */
.rept                   NUM_MSI
INTRGATE                0
                        push    $VEC
                        jmp     entry_msi
.set                    VEC, VEC + 1
.endr

entry_msi:              SAVE_STATE 2
                        mov     OFS_VEC (%rbx), %rdi
                        call    msi_vector
                        jmp     ret_from_interrupt

/*
 * IPI Entries
 */
.rept                   NUM_IPI
INTRGATE                0
                        push    $VEC
                        jmp     entry_ipi
.set                    VEC, VEC + 1
.endr

entry_ipi:              SAVE_STATE 2
                        mov     OFS_VEC (%rbx), %rdi
                        call    ipi_vector
                        jmp     ret_from_interrupt

/*
 * Interrupt Return
 */
ret_from_interrupt:     testb   $3, OFS_CS (%rbx)
                        mov     current, %rdi           // ARG_1 = Ec::current
                        jnz     ret_user_iret
                        LOAD_GPR
                        IRET

/*
 * System-Call Entry
 */
.align                  4, 0x90
.globl                  entry_sys
entry_sys:              mov     %rsp, %r11
                        mov     tss_run + 4, %rsp
                        lea     -(__SIZEOF_POINTER__ * 8)(%rsp), %rsp
                        SAVE_GPR
                        lea     STCK_TOP, %rsp
                        mov     %rdi, %rsi
                        mov     current, %rdi           // ARG_1 = Ec::current
                        and     $0xf, %rsi
                        jmp     *syscall(, %rsi, __SIZEOF_POINTER__)

/*
 * VMX Entry
 */
.align                  4, 0x90
.globl                  entry_vmx
entry_vmx:              SAVE_GPR
                        mov     %cr2, %rax
                        mov     %rax, OFS_CR2 (%rsp)
                        lea     STCK_TOP, %rsp
                        jmp     vmx_handler
