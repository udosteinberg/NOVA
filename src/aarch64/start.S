/*
 * Startup Code
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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
#include "memattr.hpp"
#include "memory.hpp"

.globl                  __start_bsp

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

__start_bsp:            mrs     x22, cntpct_el0

                        // Clear to PoC
                        mov     x0, LOAD_ADDR
                        adrp    x1, LOAD_STOP
                        sub     x1, x1, x0
                        DCACHE  cvac

1:                      msr     daifset, #0xf
                        msr     spsel,   #0x1

                        // Enable I$, D$, Disable MMU
                        mrs     x0, sctlr_el2
                        orr     x0, x0, #SCTLR_ALL_I
                        orr     x0, x0, #SCTLR_ALL_C
                        bic     x0, x0, #SCTLR_ALL_M
                        msr     sctlr_el2, x0
                        isb

                        // Zero BSS
                        adr     x0, ZERO_ADDR
                        adrp    x1, ZERO_STOP
1:                      str     xzr, [x0], #SIZE
                        cmp     x0, x1
                        blo     1b

                        // Determine PA range
                        mrs     x21, id_aa64mmfr0_el1
                        and     x21, x21, #7

                        // Determine current execution level
                        mrs     x20, currentel

                        // Check if we launched in EL3
                        cmp     x20, #PSTATE_A64_EL3
                        beq     el3

                        // Check if we launched in EL2
                        cmp     x20, #PSTATE_A64_EL2
                        beq     el2

                        // We launched in EL1 or EL0
                        ret

el3:
                        // Configure EL3 TCR: 4K pages, 48bit VA, HW-supported PA
                        mov     x0, #(TCR_A64_TG0_4K | TCR_ALL_SH0_IS | TCR_ALL_ORGN0_WB_WA | TCR_ALL_IRGN0_WB_WA | TCR_A64_T0SZ (48))
                        orr     x2, x0, x21, lsl #16
                        msr     tcr_el3, x2

                        // Configure EL3 MAIR
                        ldr     x0, =MAIR_VAL
                        msr     mair_el3, x0

                        // Configure EL3 execution controls via SCTLR
                        mrs     x0, sctlr_el3
                        orr     x0, x0, #(SCTLR_A64_SA | SCTLR_ALL_C | SCTLR_ALL_A)
                        orr     x0, x0, #(SCTLR_ALL_I)
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
                        mov     x0, #(PSTATE_A64_D | PSTATE_ALL_A | PSTATE_ALL_I | PSTATE_ALL_F | PSTATE_A64_EL2 | PSTATE_A64_SP)
                        adr     x1, el2
                        msr     spsr_el3, x0
                        msr     elr_el3, x1
                        eret

el2:
                        // Invalidate stale cache lines
                        adr     x0, ZERO_ADDR
                        adrp    x1, ZERO_STOP
                        sub     x1, x1, x0
                        DCACHE  ivac

                        // Configure EL2 TCR: 4K pages, 48bit VA, HW-supported PA
                        mov     x0, #(TCR_A64_TG0_4K | TCR_ALL_SH0_IS | TCR_ALL_ORGN0_WB_WA | TCR_ALL_IRGN0_WB_WA | TCR_A64_T0SZ (48))
                        orr     x0, x0, #(1 << 31)
                        orr     x0, x0, #(1 << 23)
                        orr     x2, x0, x21, lsl #16
                        msr     tcr_el2, x2

                        // Configure EL2 VTCR: 4K pages, IPA, HW-supported PA
                        // XXX: IPA should also depend on HW-supported PA
                        mov     x0, #(TCR_A64_TG0_4K | TCR_ALL_SH0_IS | TCR_ALL_ORGN0_WB_WA | TCR_ALL_IRGN0_WB_WA | VTCR_SL0_L1 | VTCR_T0SZ (IPA_BITS))
                        orr     x0, x0, #VTCR_RES1
                        orr     x2, x0, x21, lsl #16
                        msr     vtcr_el2, x2

                        // Configure EL2 MAIR
                        ldr     x0, =MAIR_VAL
                        msr     mair_el2, x0

                        // Configure EL2 execution controls via SCTLR
                        mrs     x0, sctlr_el2
                        orr     x0, x0, #(SCTLR_A64_SA | SCTLR_ALL_C | SCTLR_ALL_A)
                        orr     x0, x0, #(SCTLR_ALL_I)
                        msr     sctlr_el2, x0
                        isb

                        // Jump high
                        b       1f

.text

1:
                        // Switch to boot stack
                        ldr     x0, =STACK
                        mov     sp, x0

                        // General initialization
                        bl      init

                        b       .
