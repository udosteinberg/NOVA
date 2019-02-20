/*
 * Startup Code
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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
#include "selectors.hpp"

.globl                  __start_bsp

/*
 * Macros
 */
.macro                  SET_PTE LEVEL, TABLE, VIRT, PHYS, ATTR
                        mov     $((\PHYS) + (\ATTR)), %ecx
                        mov     %ecx, (\TABLE) + SIZE * ((\VIRT) >> ((\LEVEL) * PTE_BPL + PAGE_BITS) & ((1 << PTE_BPL) - 1))
.endm

.macro                  INIT_STATE
                        mov     $SEL_KERN_DATA, %ecx
                        mov     %ecx, %ss
                        mov     $SEL_USER_DATA, %ecx
                        mov     %ecx, %ds
                        mov     %ecx, %es
                        mov     %ecx, %fs
                        mov     %ecx, %gs
                        mov     $STACK, %esp
.endm

.macro                  INIT_PAGING
                        mov     $0xc0000080, %ecx
                        rdmsr
                        or      $0x101, %eax
                        wrmsr
                        mov     $0x6b8, %edx
                        mov     %edx, %cr4
                        mov     $PDBR, %edx
                        mov     %edx, %cr3
                        mov     $0x8001002b, %edx
                        mov     %edx, %cr0
.endm

/*
 * Initialization Code
 */
.section .init

/*
 * Multiboot Header
 */
__boot_header:          .long   0x1badb002
                        .long   0x0
                        .long  -0x1badb002

/*
 * Global Descriptor Table (GDT)
 */
.align                  8, 0x90
__boot_gdt:             .word   0
                        .word   __boot_gdt__ - __boot_gdt - 1
                        .long   __boot_gdt
                        .quad   0x00a09b0000000000
                        .quad   0x00a0930000000000
                        .quad   0x00a0fb0000000000
                        .quad   0x00a0f30000000000
__boot_gdt__:

/*
 * AP Startup Code
 */
.code16

__start_ap:             INIT_PAGING
                        lgdtl   %cs:__gdt_desc - __start_ap
                        ljmpl   $SEL_KERN_CODE, $__start_all

__gdt_desc:             .word   __boot_gdt__ - __boot_gdt - 1
                        .long   __boot_gdt

/*
 * BSP Startup Code
 */
.code32

__start_bsp:            mov     $(__start_bsp - __start_ap), %ecx
                        mov     $__start_ap, %esi
                        mov     $0x1000, %edi
                        rep     movsb

                        SET_PTE 3, LVL3S, LOAD_ADDR, LVL2L, 0x27
                        SET_PTE 2, LVL2L, LOAD_ADDR, LVL1L, 0x27

                        SET_PTE 3, LVL3S, LINK_ADDR, LVL2H, 0x27
                        SET_PTE 2, LVL2H, LINK_ADDR, LVL1H, 0x27

                        mov     $0xe3, %ecx
                        mov     $LVL1L, %edi
1:                      mov     %ecx, (%edi)
                        add     $(1 << (PTE_BPL + PAGE_BITS)), %ecx
                        add     $SIZE, %edi
                        cmp     $LOAD_STOP, %ecx
                        jb      1b

                        mov     $(LOAD_ADDR + 0x1e3), %ecx
                        mov     $(LVL1H + SIZE * (LINK_ADDR >> (PTE_BPL + PAGE_BITS) & ((1 << PTE_BPL) - 1))), %edi
1:                      mov     %ecx, (%edi)
                        add     $(1 << (PTE_BPL + PAGE_BITS)), %ecx
                        add     $SIZE, %edi
                        cmp     $LOAD_STOP, %ecx
                        jb      1b

                        INIT_PAGING
                        lgdt    __boot_gdt + 2
                        ljmp    $SEL_KERN_CODE, $__start_all

/*
 * Common Startup Code
 */
.code64

__start_all:            INIT_STATE
                        test    %REG(bx), %REG(bx)
                        je      2f

                        mov     %REG(bx), %ARG_1
                        call    init
                        jmp     3f

1:                      pause
2:                      xchg    %REG(bx), boot_lock
                        test    %REG(bx), %REG(bx)
                        je      1b

3:                      call    kern_ptab_setup
                        jmp     1f

.text

1:                      mov     %REG(ax), %cr3
                        mov     $(CPU_LOCAL_STCK + PAGE_SIZE), %REG(sp)
                        jmp     bootstrap
