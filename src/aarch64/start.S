/*
 * Startup Code
 *
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
 */

#include "arch.hpp"
#include "memory.hpp"

.global                 __init_bsp

/*
 * Macros
 */
.macro                  SWITCH  EL3, EL2, EL1
                        mrs     x0, currentel
                        cmp     x0, #SPSR_EL2
                        b.gt    \EL3
                        b.eq    \EL2
                        b.lt    \EL1
.endm

/*
 * Initialization Code
 */
.section .init

__init_bsp:
                        // Determine relocated addresses
                        adrp    x25, NOVA_HPAS
                        adrp    x26, PTAB_HPAS
                        adrp    x27, ZERO_HPAS
                        adrp    x28, NOVA_HPAE
                        mov     x29, LINK_ADDR
                        sub     x29, x29, x25

                        // Check load address alignment
                        ubfx    x2, x25, #0, #ALIGNMENT_NOVA
                        cbz     x2, .Linit_all
.Ldead:
                        // Fail boot
                        ldr     lr, =0xd00d2badc0dedead
                        ret

.Linit_all:             msr     daifset, #0xf
                        msr     spsel,   #0x1

                        // Invoke launch point of current EL
                        SWITCH  .Lel3, .Lel2, .Ldead
.Lel3:
                        // Disable EL3 MMU => I$ = Inner/Outer-WT, D$ = Device-nGnRnE
                        mov     x0, #SCTLR_I | SCTLR_EOS
                        msr     sctlr_el3, x0
                        isb

                        // Reconfigure EL3 MMU: SCR
                        ldr     x0, =SCR_NOVA
                        msr     scr_el3, x0

                        // Invalidate residual EL3 TLB entries
                        tlbi    alle3
                        dsb     nsh
                        isb

                        // Set EL2 initial SCTLR
                        msr     sctlr_el2, xzr

                        // Switch to EL2h
                        mov     x0, #(SPSR_D | SPSR_A | SPSR_I | SPSR_F | SPSR_EL2 | SPSR_SP)
                        adr     x1, .Lel2
                        msr     spsr_el3, x0
                        msr     elr_el3, x1
                        eret
.Lel2:
                        // Disable EL2 MMU => I$ = Inner/Outer-WT, D$ = Device-nGnRnE
                        mov     x0, #SCTLR_I | SCTLR_EOS
                        msr     sctlr_el2, x0
                        isb

                        // Reconfigure EL2 MMU: HCR
                        ldr     x0, =HCR_NOVA
                        msr     hcr_el2, x0

                        // Invalidate residual EL2 TLB entries
                        tlbi    alle2
                        dsb     nsh
                        isb

                        // Switch to boot stack
                        adr     x0, STACK
                        mov     sp, x0

                        // Jump high
                        ldr     x0, =.Lhigh
                        br      x0

.text

.Lhigh:
                        // General initialization
                        bl      init

                        b       .
