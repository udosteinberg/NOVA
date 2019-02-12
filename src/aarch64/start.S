/*
 * Startup Code
 *
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

1:                      msr     daifset, #0xf
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

                        // Invalidate stale cache lines
                        adr     x0, ZERO_HPAS
                        sub     x1, x23, x0
                        DCACHE  ivac

                        // Switch to boot stack
                        adr     x0, STACK
                        mov     sp, x0

                        // General initialization
                        bl      init

                        b       .
