/*
 * Startup Code
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
 */

#include "arch.hpp"
#include "memory.hpp"
#include "selectors.hpp"

/*
 * Initialization Code
 */
.section .init

.globl                  __start_bsp

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

.macro                  INIT_STATE
                        mov     $SEL_KERN_DATA, %ecx
                        mov     %ecx, %ss
                        xor     %ecx, %ecx
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

                        mov     $(LVL3L + 0x27), %ecx
                        mov     %ecx, LVL4  + __SIZEOF_POINTER__ * (LOAD_ADDR >> (3 * PTE_BPL + PAGE_BITS) & ((1 << PTE_BPL) - 1))
                        mov     $(LVL2L + 0x27), %ecx
                        mov     %ecx, LVL3L + __SIZEOF_POINTER__ * (LOAD_ADDR >> (2 * PTE_BPL + PAGE_BITS) & ((1 << PTE_BPL) - 1))

                        mov     $(LVL3H + 0x27), %ecx
                        mov     %ecx, LVL4  + __SIZEOF_POINTER__ * (LINK_ADDR >> (3 * PTE_BPL + PAGE_BITS) & ((1 << PTE_BPL) - 1))
                        mov     $(LVL2H + 0x27), %ecx
                        mov     %ecx, LVL3H + __SIZEOF_POINTER__ * (LINK_ADDR >> (2 * PTE_BPL + PAGE_BITS) & ((1 << PTE_BPL) - 1))

                        mov     $0xe3, %ecx
                        mov     $LVL2L, %edi
1:                      mov     %ecx, (%edi)
                        add     $(1 << (PTE_BPL + PAGE_BITS)), %ecx
                        add     $__SIZEOF_POINTER__, %edi
                        cmp     $LOAD_E, %ecx
                        jb      1b

                        mov     $(LINK_P + 0x1e3), %ecx
                        mov     $(LVL2H + __SIZEOF_POINTER__ * (LINK_ADDR >> (PTE_BPL + PAGE_BITS) & ((1 << PTE_BPL) - 1))), %edi
1:                      mov     %ecx, (%edi)
                        add     $(1 << (PTE_BPL + PAGE_BITS)), %ecx
                        add     $__SIZEOF_POINTER__, %edi
                        cmp     $LINK_E, %ecx
                        jb      1b

                        INIT_PAGING
                        lgdt    __boot_gdt + 2
                        ljmp    $SEL_KERN_CODE, $__start_all

/*
 * Common Startup Code
 */
.code64

__start_all:            INIT_STATE
                        test    %rbx, %rbx
                        je      2f

                        mov     %rbx, %rdi
                        call    init
                        jmp     3f

1:                      pause
2:                      xchg    %rbx, boot_lock
                        test    %rbx, %rbx
                        je      1b

3:                      call    kern_ptab_setup
                        jmp     1f

.text

1:                      mov     %rax, %cr3
                        mov     $(CPU_LOCAL_STCK + PAGE_SIZE), %rsp
                        jmp     bootstrap
