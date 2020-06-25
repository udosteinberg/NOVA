/*
 * Entry Functions
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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
 *
 */

#include "arch.hpp"
#include "hazards.hpp"
#include "memory.hpp"
#include "selectors.hpp"
#include "vectors.hpp"

.data
.global                 handlers
handlers:

.text

.macro                  INTRGATE DPL
                        .align  4, 0x90
1:                      .data
                         WORD   1b + \DPL
                        .previous
.endm

.macro                  TASKGATE
                        .data
                         WORD   0
                        .previous
.endm

.macro                  SAVE_STATE
                        SAVE_SEG
                        SAVE_GPR
                        mov     %REG(sp), %REG(bx)
                        cmp     $CPU_LOCAL_STCK, %REG(sp)
                        jae     1f
                        cld
                        mov     $CPU_LOCAL_STCK + PAGE_SIZE, %REG(sp)
                        mov     $SEL_USER_DATA, %REG(ax)
                        mov     %REG(ax), %ds
                        mov     %REG(ax), %es
1:
.endm

.macro                  EXCEPTION VEC, DPL
INTRGATE                \DPL
                        push    $\VEC
                        jmp     entry_exc
.endm

.macro                  EXCEPTION_ERROR VEC, DPL
INTRGATE                \DPL
                        push    (%REG(sp))
                        movq    $\VEC, 8(%rsp)
                        jmp     entry_exc_error
.endm

/*
 * Exception Entries
 */
entry_exc:              push    $0
entry_exc_error:        SAVE_STATE
                        mov     %cr2, %REG(ax)
                        mov     %REG(ax), OFS_CR2 (%REG(bx))
                        mov     %REG(bx), %ARG_1
                        call    exc_handler
                        jmp     ret_from_interrupt

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
 * GSI Entries
 */
.set                    VEC, NUM_EXC
.rept                   NUM_GSI
INTRGATE                0
                        push    $VEC
                        jmp     entry_gsi
.set                    VEC, VEC + 1
.endr

entry_gsi:              push    $0
                        SAVE_STATE
                        mov     OFS_VEC (%REG(bx)), %ARG_1
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

entry_lvt:              push    $0
                        SAVE_STATE
                        mov     OFS_VEC (%REG(bx)), %ARG_1
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

entry_msi:              push    $0
                        SAVE_STATE
                        mov     OFS_VEC (%REG(bx)), %ARG_1
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

entry_ipi:              push    $0
                        SAVE_STATE
                        mov     OFS_VEC (%REG(bx)), %ARG_1
                        call    ipi_vector
                        jmp     ret_from_interrupt

/*
 * Interrupt Return
 */
ret_from_interrupt:     testb   $3, OFS_CS (%REG(bx))
                        jnz     ret_user_iret
                        LOAD_GPR
                        add     $(6 * SIZE), %REG(sp)
                        RET_USER_EXC

/*
 * System-Call Entry
 */
.align                  4, 0x90
.globl                  entry_sysenter
entry_sysenter:         LOAD_KSP
                        lea     -11*SIZE(%REG(sp)), %REG(sp)
                        SAVE_GPR
                        mov     $(CPU_LOCAL_STCK + PAGE_SIZE), %REG(sp)
                        and     $0xf, %ARG_1
                        jmp     *syscall(,%ARG_1,SIZE)

/*
 * VMX Entry
 */
.align                  4, 0x90
.globl                  entry_vmx
entry_vmx:              SAVE_GPR
                        mov     %cr2, %REG(ax)
                        mov     %REG(ax), OFS_CR2 (%REG(sp))
                        mov     $(CPU_LOCAL_STCK + PAGE_SIZE), %REG(sp)
                        jmp     vmx_handler
