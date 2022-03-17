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
#include "memattr.hpp"
#include "memory.hpp"

#define PTE_ATTR_P      BIT  (0)    // Present
#define PTE_ATTR_nL     BIT  (1)    // Not Large
#define PTE_ATTR_CA(X)  ((X) << 2)  // Cacheability
#define PTE_ATTR_SH(X)  ((X) << 8)  // Shareability
#define PTE_ATTR_A      BIT (10)    // Accessed
#define PTE_ATTR_nG     BIT (11)    // Not Global

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

.macro                  DCSIZE  L, S, B
                        mrs     x0, \S
                        ubfx    x0, x0, \B, #4
                        mov     \L, #4
                        lsl     \L, \L, x0
.endm

.macro                  DCACHE  O, L, S, E, B
                        sub     x1, \L, #1
                        bic     x0, \S, x1
                    .ifnb \B
                        dmb     \B                  // prevent reordering of cache OP with earlier LD/ST
                    .endif
0:                      dc      \O, x0
                        add     x0, x0, \L
                        cmp     x0, \E
                        blo     0b
                    .ifnb \B
                        dmb     \B                  // prevent reordering of cache OP with later LD/ST
                    .endif
.endm

.macro                  PTE_FIX L, PTAB, VIRT, PHYS, ATTR
                        adr     x0, (\PTAB) + __SIZEOF_POINTER__ * ((\VIRT) >> LEVL_BITS (\L) & (BIT (PTE_BPL) - 1))
                        adr     x1, (\PHYS) + (\ATTR)
                        str     x1, [x0]
.endm

.macro                  PTE_REL L, PTAB, VIRT, PHYS, ATTR
                        adr     x0, (\PTAB)
                        adr     x1, (\PHYS) + (\ATTR)
                        adr     x2, (\VIRT)
                        lsr     x2, x2, LEVL_BITS (\L)
                        and     x2, x2, BIT (PTE_BPL) - 1
                        add     x0, x0, x2, lsl #3
                        str     x1, [x0]
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
                        cbz     x2, 1f
.Ldead:
                        // Fail boot
                        ldr     lr, =0xd00d2badc0dedead
                        ret
1:
                        // Determine SCTLR of current EL
                        SWITCH  .Lsctlr_el3, .Lsctlr_el2, .Lsctlr_el1
.Lsctlr_el3:            mrs     x22, sctlr_el3
                        b       .Lcache
.Lsctlr_el2:            mrs     x22, sctlr_el2
                        b       .Lcache
.Lsctlr_el1:            mrs     x22, sctlr_el1

                        // Determine D$ state
.Lcache:                ands    xzr, x22, #SCTLR_C
                        and     x22, x22, #SCTLR_M
                        csel    x22, x22, xzr, ne

                        // Determine D$ sizes
                        DCSIZE  x23, ctr_el0, #16
                        DCSIZE  x24, dczid_el0, #0

                        // Clean image to PoC
                        DCACHE  cvac, x23, x25, x26, sy

                        // If D$ is bypassed then discard dirty lines in the PTAB memory region
                        cbnz    x22, .Lskip_dirty
                        DCACHE  ivac, x23, x26, x27, sy
.Lskip_dirty:
                        // Zero PTAB memory region
                        mov     x0, x26
                        mov     x1, x27
.Lzero_loop:            stp     xzr, xzr, [x0], #__SIZEOF_POINTER__ * 2
                        cmp     x0, x1
                        blo     .Lzero_loop

                        // Configure boot page tables: Load Section (2M page)
                        PTE_REL  3, PT3S_HPAS, NOVA_HPAS, PT2L_HPAS, PTE_ATTR_nL | PTE_ATTR_P
                        PTE_REL  2, PT2L_HPAS, NOVA_HPAS, PT1L_HPAS, PTE_ATTR_nL | PTE_ATTR_P
                        PTE_REL  1, PT1L_HPAS, NOVA_HPAS, NOVA_HPAS, PTE_ATTR_nG | PTE_ATTR_A | PTE_ATTR_SH (3) | PTE_ATTR_CA (CA_MEM_WB) | PTE_ATTR_P

                        // Configure boot page tables: Link Section (2M pages)
                        PTE_FIX  3, PT3S_HPAS, LINK_ADDR, PT2H_HPAS, PTE_ATTR_nL | PTE_ATTR_P
                        PTE_FIX  2, PT2H_HPAS, LINK_ADDR, PT1H_HPAS, PTE_ATTR_nL | PTE_ATTR_P

                        adr     x1, PT1H_HPAS + __SIZEOF_POINTER__ * (LINK_ADDR >> LEVL_BITS (1) & (BIT (PTE_BPL) - 1))
                        adr     x0, NOVA_HPAS + (PTE_ATTR_nG | PTE_ATTR_A | PTE_ATTR_SH (3) | PTE_ATTR_CA (CA_MEM_WB) | PTE_ATTR_P)
.Lptab_loop:            str     x0, [x1], #__SIZEOF_POINTER__
                        add     x0, x0, #PAGE_SIZE (1)
                        cmp     x0, x28
                        blo     .Lptab_loop
                        dsb     ishst

                        // If D$ is bypassed then discard stale lines in the PTAB memory region
                        cbnz    x22, .Lskip_stale
                        DCACHE  ivac, x23, x26, x27, sy
.Lskip_stale:

.Linit_all:             msr     daifset, #0xf
                        msr     spsel,   #0x1

                        // Determine PA range (0-5)
                        mov     x0, #5
                        mrs     x20, id_aa64mmfr0_el1
                        ubfx    x20, x20, #0, #4
                        cmp     x20, x0
                        csel    x20, x0, x20, hi

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

                        // Reconfigure EL3 MMU: MAIR
                        ldr     x0, =MAIR_NOVA
                        msr     mair_el3, x0

                        // Reconfigure EL3 MMU: TCR = 4K pages, 48bit VA, HW-supported PA
                        mov     x0, #(TCR_TG0_4K | TCR_SH0_INNER | TCR_ORGN0_WB_RW | TCR_IRGN0_WB_RW | TCR_T0SZ (48))
                        bfi     x0, x20, #16, #3
                        msr     tcr_el3, x0

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

                        // Reconfigure EL2 MMU: MAIR
                        ldr     x0, =MAIR_NOVA
                        msr     mair_el2, x0

                        // Reconfigure EL2 MMU: TCR = 4K pages, 48bit VA, HW-supported PA
                        mov     x0, #(TCR_TG0_4K | TCR_SH0_INNER | TCR_ORGN0_WB_RW | TCR_IRGN0_WB_RW | TCR_T0SZ (48))
                        bfi     x0, x20, #16, #3
                        msr     tcr_el2, x0

                        // Reconfigure EL2 MMU: TTBR
                        adrp    x0, PTAB_HPAS
                        msr     ttbr0_el2, x0

                        // Invalidate residual EL2 TLB entries
                        tlbi    alle2
                        dsb     nsh
                        isb

                        // Enable EL2 MMU => I$/D$ cacheability defined by PTE
                        ldr     x0, =SCTLR_I | SCTLR_EOS | SCTLR_SA0 | SCTLR_SA | SCTLR_C | SCTLR_A | SCTLR_M
                        msr     sctlr_el2, x0
                        isb

                        // Zero BSS/HEAP using VA
                        add     x3, x27, x29
                        add     x4, x28, x29
                        DCACHE  zva, x24, x3, x4

                        // Initialize kmem offset
                        adr     x0, __kmem_offset - OFFSET
                        str     x29, [x0]

                        // Switch to boot stack
                        adr     x0, STACK
                        mov     sp, x0

                        // Jump high
                        ldr     x0, =.Lhigh
                        br      x0

.text

.Lhigh:
                        // Early initialization
                        bl      preinit

                        // General initialization
                        bl      init

                        b       .
