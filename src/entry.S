/*
 * Entry Functions
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "memory.h"
#include "selectors.h"
#include "vectors.h"

.data
.global                 handlers
handlers:

.text

.macro                  INTRGATE DPL
                        .align  4, 0x90
1:                      .data
                        .long   1b + \DPL
                        .previous
.endm

.macro                  TASKGATE
                        .data
                        .long   0
                        .previous
.endm

.macro                  SAVE_STATE
                        push    %ds
                        push    %es
                        push    %fs
                        push    %gs
                        pusha
                        mov     %esp, %ebx
                        cmp     $KSTCK_ADDR, %esp
                        jae     1f
                        cld
                        mov     $KSTCK_ADDR + PAGE_SIZE, %esp
                        mov     $SEL_USER_DATA, %eax
                        mov     %eax, %ds
                        mov     %eax, %es
1:
.endm

.macro                  EXCEPTION VEC, DPL
INTRGATE                \DPL
                        push    $\VEC
                        jmp     entry_exc
.endm

.macro                  EXCEPTION_ERROR VEC, DPL
INTRGATE                \DPL
                        push    (%esp)
                        movl    $\VEC, 4(%esp)
                        jmp     entry_exc_error
.endm

/*
 * Exception Entries
 */
entry_exc:              push    $0
entry_exc_error:        SAVE_STATE
                        mov     %cr2, %eax
                        mov     %eax, 0xc(%ebx)
                        mov     %ebx, %eax
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
TASKGATE
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
                        mov     0x34(%ebx), %eax
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
                        mov     0x34(%ebx), %eax
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
                        mov     0x34(%ebx), %eax
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
                        mov     0x34(%ebx), %eax
                        call    ipi_vector
                        jmp     ret_from_interrupt

/*
 * Interrupt Return
 */
ret_from_interrupt:     testb   $3, 0x3c(%ebx)
                        jnz     ret_user_iret
                        popa
                        add     $24, %esp
                        iret

/*
 * System-Call Entry
 */
.align                  4, 0x90
.globl                  entry_sysenter
entry_sysenter:         cld
                        pop     %esp
                        lea     -44(%esp), %esp
                        pusha
                        mov     $(KSTCK_ADDR + PAGE_SIZE), %esp
                        and     $0xf, %eax
                        jmp     *syscall(,%eax,4)

/*
 * VMX Entry
 */
.align                  4, 0x90
.globl                  entry_vmx
entry_vmx:              pusha
                        mov     %cr2, %eax
                        mov     %eax, 0xc(%esp)
                        mov     $(KSTCK_ADDR + PAGE_SIZE), %esp
                        jmp     vmx_handler

/*
 * SVM Entry
 */
.align                  4, 0x90
.globl                  entry_svm
entry_svm:              cli
                        stgi
                        pusha
                        mov     $(KSTCK_ADDR + PAGE_SIZE), %esp
                        jmp     svm_handler
