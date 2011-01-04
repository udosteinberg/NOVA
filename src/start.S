/*
 * Startup Code
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
 */

#include "memory.h"
#include "selectors.h"

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
                        .quad   0x00cf9b000000ffff      /* Kernel Code */
                        .quad   0x00cf93000000ffff      /* Kernel Data */
                        .quad   0x00cffb000000ffff      /* User Code */
                        .quad   0x00cff3000000ffff      /* User Data */
__boot_gdt__:

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
                        mov     $0x698, %edx
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

                        mov     $0xe3, %eax
                        mov     $PDBR, %edi
1:                      stosl
                        add     $0x400000, %eax
                        cmp     $LOAD_E, %eax
                        jb      1b

                        mov     $(LINK_P + 0x1e3), %eax
                        mov     $(PDBR + (LINK_ADDR >> 20)), %edi
1:                      stosl
                        add     $0x400000, %eax
                        cmp     $LINK_E, %eax
                        jb      1b

                        INIT_PAGING
                        lgdt    __boot_gdt + 2
                        ljmp    $SEL_KERN_CODE, $__start_all

/*
 * Common Startup Code
 */
__start_all:            INIT_STATE
                        test    %ebx, %ebx
                        je      2f

                        mov     %ebx, %eax
                        call    init
                        jmp     3f

1:                      pause
2:                      xchg    %ebx, boot_lock
                        test    %ebx, %ebx
                        je      1b

3:                      call    kern_ptab_setup
                        jmp     1f

.text

1:                      mov     %eax, %cr3
                        mov     $(KSTCK_ADDR + PAGE_SIZE), %esp
                        jmp     bootstrap
