/*
 * Entry Functions
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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
#include "memory.hpp"
#include "vectors.hpp"

.data
.global                 handlers
handlers:

.text

.macro                  NOGATE
                        .data
                        .quad   0
                        .previous
.endm

.macro                  INTRGATE IDT
                        .balign 4, 0x90
1:
                        .data
                        .quad   1b + \IDT
                        .previous
.endm

.macro                  EXCEPTION_ERROR VEC, IDT
INTRGATE                \IDT
                        push    (%rsp)
                        movq    $\VEC, __SIZEOF_POINTER__(%rsp)
    .if \IDT & IDT_IST1
                        jmp     entry_ist_err
    .else
                        jmp     entry_exc_err
    .endif
.endm

.macro                  EXCEPTION VEC, IDT
INTRGATE                \IDT
                        push    $\VEC
    .if \IDT & IDT_IST1
                        jmp     entry_ist
    .else
                        jmp     entry_exc
    .endif
.endm

.macro                  INTERRUPT VEC, LABEL
INTRGATE                0
                        push    $\VEC
                        jmp     \LABEL
.endm

.macro                  SAVE_STATE
                        SAVE_GPR
                        mov     %rsp, %rbx
                        testb   $3, OFS_CS(%rbx)
                        jz      1f
                        cld
                        lea     CPU_LOCAL_STCK + PAGE_SIZE, %rsp
1:
.endm

/*
 * Exceptions
 */
EXCEPTION               EXC_DE,     0               // Fault
EXCEPTION               EXC_DB,     0               // Trap or Fault
EXCEPTION               EXC_NMI,    IDT_IST1        // NMI
EXCEPTION               EXC_BP,     IDT_USER        // Trap
EXCEPTION               EXC_OF,     IDT_USER        // Trap
EXCEPTION               EXC_BR,     0               // Fault
EXCEPTION               EXC_UD,     0               // Fault
EXCEPTION               EXC_NM,     0               // Fault
EXCEPTION_ERROR         EXC_DF,     IDT_IST1        // Abort
NOGATE                                              // Abort
EXCEPTION_ERROR         EXC_TS,     0               // Fault
EXCEPTION_ERROR         EXC_NP,     0               // Fault
EXCEPTION_ERROR         EXC_SS,     0               // Fault
EXCEPTION_ERROR         EXC_GP,     0               // Fault
EXCEPTION_ERROR         EXC_PF,     0               // Fault
NOGATE
EXCEPTION               EXC_MF,     0               // Fault
EXCEPTION_ERROR         EXC_AC,     0               // Fault
EXCEPTION               EXC_MC,     IDT_IST1        // Abort
EXCEPTION               EXC_XM,     0               // Fault
EXCEPTION               EXC_VE,     0               // Fault
EXCEPTION_ERROR         EXC_CP,     0               // Fault

/*
 * Unused Exceptions
 */
.rept                   NUM_EXC - 0x16
NOGATE
.endr

/*
 * Exception Entry via IST
 */
entry_ist:              push    $0                      // ERR
entry_ist_err:          SAVE_GPR                        // GPR
                        cld
                        mov     %rsp, %rdi              // ARG1 = Exc_regs
                        call    ist_handler
                        LOAD_GPR
                        IRET

/*
 * Exception Entry
 */
entry_exc:              push    $0                      // ERR
entry_exc_err:          SAVE_STATE                      // GPR
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
                        push    $0                      // ERR
                        SAVE_STATE                      // GPR
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
.balign                 4, 0x90
.global                 entry_sys
entry_sys:              mov     %rsp, %r11
                        mov     tss_run + 4, %rsp
                        lea     -7 * __SIZEOF_POINTER__(%rsp), %rsp
                        SAVE_GPR
                        and     $0xf, %rdi
                        lea     CPU_LOCAL_STCK + PAGE_SIZE - __SIZEOF_POINTER__, %rsp
                        jmp     *syscall(, %rdi, __SIZEOF_POINTER__)

/*
 * VMX Entry
 */
.balign                 4, 0x90
.global                 entry_vmx
entry_vmx:              SAVE_GPR
                        lea     CPU_LOCAL_STCK + PAGE_SIZE - __SIZEOF_POINTER__, %rsp
                        jmp     vmx_handler
