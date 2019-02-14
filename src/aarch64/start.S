/*
 * Startup Code
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

.globl                  __init_bsp

/*
 * Macros
 */
.macro                  DCACHE  OP
                        mrs     x3, ctr_el0
                        ubfm    x3, x3, #16, #19
                        mov     x2, #4
                        lsl     x2, x2, x3
                        add     x1, x0, x1
                        sub     x3, x2, #1
                        bic     x0, x0, x3
1:                      dc     \OP, x0
                        add     x0, x0, x2
                        cmp     x0, x1
                        blo     1b
                        dsb     sy
.endm

/*
 * Initialization Code
 */
.section .init

__init_bsp:
                        // Determine image boundaries
                        adrp    x22, NOVA_HPAS
                        adrp    x23, NOVA_HPAE
                        mov     x24, LINK_ADDR
                        sub     x24, x24, x22

                        // Refuse invalid load address
                        and     x2, x22, #OFFS_MASK (1)
                        cbnz    x2, .

                        // Clean image to PoC
                        mov     x0, x22
                        sub     x1, x23, x22
                        DCACHE  cvac

                        msr     daifset, #0xf
                        msr     spsel,   #0x1

                        // Enable I$, D$, Disable MMU
                        mrs     x0, sctlr_el2
                        orr     x0, x0, #SCTLR_I
                        orr     x0, x0, #SCTLR_C
                        bic     x0, x0, #SCTLR_M
                        msr     sctlr_el2, x0
                        isb

                        // Zero BSS
                        adr     x0, ZERO_HPAS
.Lbss_loop:             str     xzr, [x0], #__SIZEOF_POINTER__
                        cmp     x0, x23
                        blo     .Lbss_loop

                        // Determine current execution level
                        mrs     x20, currentel

                        // Check if we launched in EL3
                        cmp     x20, #SPSR_EL3
                        beq     .Lel3

                        // Check if we launched in EL2
                        cmp     x20, #SPSR_EL2
                        beq     .Lel2

                        // We launched in EL1 or EL0
                        ret
.Lel3:
                        // Configure EL3 execution controls via SCTLR
                        mrs     x0, sctlr_el3
                        orr     x0, x0, #(SCTLR_SA | SCTLR_C | SCTLR_A)
                        orr     x0, x0, #(SCTLR_I)
                        msr     sctlr_el3, x0
                        isb

                        // Configure EL2 execution controls via SCR
                        mrs     x0, scr_el3
                        orr     x0, x0, #SCR_RW
                        orr     x0, x0, #SCR_RES1
                        orr     x0, x0, #SCR_NS
                        msr     scr_el3, x0

                        // Configure EL2 initial SCTLR
                        msr     sctlr_el2, xzr

                        // Return to EL2
                        mov     x0, #(SPSR_D | SPSR_A | SPSR_I | SPSR_F | SPSR_EL2 | SPSR_SP)
                        adr     x1, .Lel2
                        msr     spsr_el3, x0
                        msr     elr_el3, x1
                        eret
.Lel2:
                        // Invalidate stale cache lines
                        adr     x0, ZERO_HPAS
                        sub     x1, x23, x0
                        DCACHE  ivac

                        // Configure EL2 execution controls via SCTLR
                        mrs     x0, sctlr_el2
                        orr     x0, x0, #(SCTLR_SA | SCTLR_C | SCTLR_A)
                        orr     x0, x0, #(SCTLR_I)
                        msr     sctlr_el2, x0
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
