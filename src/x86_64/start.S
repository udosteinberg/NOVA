/*
 * Startup Code
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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
#include "memattr.hpp"
#include "memory.hpp"
#include "selectors.hpp"

#define PTE_ATTR_P      BIT (0)     // Present
#define PTE_ATTR_W      BIT (1)     // Writable
#define PTE_ATTR_U      BIT (2)     // User
#define PTE_ATTR_A      BIT (5)     // Accessed
#define PTE_ATTR_D      BIT (6)     // Dirty
#define PTE_ATTR_S      BIT (7)     // Superpage
#define PTE_ATTR_G      BIT (8)     // Global

.globl                  __init_bsp, __init_aps, __init_aps__, __wake_vec

/*
 * Macros
 */
.macro                  PTE_FIX L, PTAB, VIRT, PHYS, ATTR
                        lea     (\PHYS) + (\ATTR)(%ebp), %ecx
                        mov     %ecx, (\PTAB) + __SIZEOF_POINTER__ * ((\VIRT) >> ((\L) * PTE_BPL + PAGE_BITS) & (BIT (PTE_BPL) - 1))(%ebp)
.endm

.macro                  PTE_REL L, PTAB, VIRT, PHYS, ATTR
                        lea     (\PHYS) + (\ATTR)(%ebp), %ecx
                        lea     (\VIRT)(%ebp), %edx
                        shr     $((\L) * PTE_BPL + PAGE_BITS), %edx
                        and     $(BIT (PTE_BPL) - 1), %edx
                        mov     %ecx, (\PTAB)(%ebp, %edx, __SIZEOF_POINTER__)
.endm

.macro                  INIT_PAGING
                        mov     $0x277, %ecx
                        mov     $(PAT_VAL >> 32), %edx
                        mov     $(PAT_VAL & BIT_RANGE (31, 0)), %eax
                        wrmsr
                        mov     $0xc0000080, %ecx
                        xor     %edx, %edx
                        mov     $(EFER_NXE | EFER_LME | EFER_SCE), %eax
                        wrmsr
                        lea     STACK(%ebp), %esp
                        lea     PT3S_HPAS(%ebp), %ecx
                        mov     $(CR4_OSXMMEXCPT | CR4_OSFXSR | CR4_PGE | CR4_PAE | CR4_DE), %edx
                        mov     $(CR0_PG | CR0_WP | CR0_NE | CR0_TS | CR0_MP | CR0_PE), %eax
                        mov     %edx, %cr4
                        mov     %ecx, %cr3
                        mov     %eax, %cr0
.endm

.macro                  REL_SYM SYMBOL
                        add     %ebp, \SYMBOL(%ebp)
.endm

/*
 * Initialization Code
 */
.section .init

/*
 * 32/64-bit Startup Code: Bootstrap Processor (BSP)
 */
.code32

__init_bsp:             // Determine relocation offset
                        xchg    %ebx, %esp
                        pop     %esi
                        call    .Lret_addr
.Lret_addr:             pop     %ebp
                        push    %esi
            dec %esi;   sub     $.Lret_addr, %ebp
                        xchg    %ebx, %esp

                        // Ensure flags are in a defined state
                        cli
                        cld

                        // Load relocated boot GDT
                        REL_SYM (__relo_pdb - OFFSET)
                        lgdt    (__relo_pdl - OFFSET)(%ebp)

                        // Configure Page Tables: Load Section (2M page)
                        PTE_REL 3, PT3S_HPAS, NOVA_HPAS, PT2L_HPAS, PTE_ATTR_A | PTE_ATTR_U | PTE_ATTR_W | PTE_ATTR_P
                        PTE_REL 2, PT2L_HPAS, NOVA_HPAS, PT1L_HPAS, PTE_ATTR_A | PTE_ATTR_U | PTE_ATTR_W | PTE_ATTR_P
                        PTE_REL 1, PT1L_HPAS, NOVA_HPAS, NOVA_HPAS, PTE_ATTR_S | PTE_ATTR_D | PTE_ATTR_A | PTE_ATTR_W | PTE_ATTR_P

                        // FIXME: AP Startup Code mapping
                        PTE_REL 1, PT1L_HPAS, 0, 0, PTE_ATTR_S | PTE_ATTR_D | PTE_ATTR_A | PTE_ATTR_W | PTE_ATTR_P

                        // Configure Page Tables: Link Section (2M pages)
                        PTE_FIX 3, PT3S_HPAS, LINK_ADDR, PT2H_HPAS, PTE_ATTR_A | PTE_ATTR_U | PTE_ATTR_W | PTE_ATTR_P
                        PTE_FIX 2, PT2H_HPAS, LINK_ADDR, PT1H_HPAS, PTE_ATTR_A | PTE_ATTR_U | PTE_ATTR_W | PTE_ATTR_P

                        lea     PT1H_HPAS + __SIZEOF_POINTER__ * (LINK_ADDR >> (PTE_BPL + PAGE_BITS) & (BIT (PTE_BPL) - 1))(%ebp), %edx
                        lea     NOVA_HPAS + (PTE_ATTR_G | PTE_ATTR_S | PTE_ATTR_D | PTE_ATTR_A | PTE_ATTR_W | PTE_ATTR_P)(%ebp), %ecx
                        lea     NOVA_HPAE(%ebp), %eax
.Lpte_loop:             mov     %ecx, (%edx)
                        add     $__SIZEOF_POINTER__, %edx
                        add     $PAGE_SIZE (1), %ecx
                        cmp     %eax, %ecx
                        jb      .Lpte_loop

                        // Initialize jump target
                        lea     __init_all(%ebp), %eax
                        mov     %eax,           __relo_jip - OFFSET(%ebp)
                        movw    $SEL_KERN_CODE, __relo_jcs - OFFSET(%ebp)

                        INIT_PAGING
                        ljmp   *__relo_jip - OFFSET(%ebp)

/*
 * 16-bit Startup Code: Application Processors (APs)
 */
.code16

__init_aps:             jmp     .Lskip_vec
__wake_vec:             orb     $1, %bl
.Lskip_vec:             mov     %cs:.Lrelo_pdb - __init_aps, %ebp
                        sub     $__boot_gdt, %ebp
                        INIT_PAGING
                        lgdtl   %cs:.Lrelo_pdl - __init_aps
                        ljmpl  *%cs:.Lrelo_jip - __init_aps

.Lrelo_jip:             .long   __init_all                      // Jump IP
.Lrelo_jcs:             .word   SEL_KERN_CODE                   // Jump CS
.Lrelo_pdl:             .word   .Lboot_gdt - __boot_gdt - 1     // Pseudo Descriptor Limit
.Lrelo_pdb:             .long   __boot_gdt                      // Pseudo Descriptor Base

__init_aps__:

/*
 * Global Descriptor Table (GDT)
 */
                        .balign 8
__boot_gdt:             .quad   0                               // SEL_NULL
                        .quad   0x00a09b0000000000              // SEL_KERN_CODE (64-bit)
                        .quad   0x00a0930000000000              // SEL_KERN_DATA (64-bit)
.Lboot_gdt:

/*
 * 64-bit Startup Code: All Processors
 */
.code64

__init_all:             mov     $.Lhigh, %rax
                        jmp     *%rax

.text

.Lhigh:                 mov     $SEL_KERN_DATA, %ecx
                        mov     %cx, %ss
                        xor     %ecx, %ecx
                        mov     %cx, %ds
                        mov     %cx, %es
                        mov     %cx, %fs
                        mov     %cx, %gs

                        // Preinit (BSP only)
                        test    %ebx, %ebx
                        jz      .Lskip_preinit

                        // Initialize kmem offset, based on 32-bit load skew
                        lea     NOVA_HPAS(%ebp), %rax
                        lea     LINK_ADDR, %rdi
                        sub     %rax, %rdi
                        mov     %rdi, __kmem_offset
.Lskip_preinit:
                        // Acquire boot lock
                        mov     $BIT (16), %eax
                   lock xadd    %eax, __boot_lock
                        movzwl  %ax, %edx
                        shr     $16, %eax
.Llock_retry:           cmp     %eax, %edx
                        je      .Llock_end
                        pause
                        movzwl  __boot_lock, %edx
                        jmp     .Llock_retry
.Llock_end:
                        // Init (BSP only)
                        test    %ebx, %ebx
                        jz      .Lskip_init

                        // Apply these relocations post-launch
                        REL_SYM .Lrelo_jip
                        REL_SYM .Lrelo_pdb

                        mov     %rbx, %rdi
                        call    init
.Lskip_init:
                        // Determine topology
                        xor     %eax, %eax
                        cpuid
                        mov     $0xb, %esi
                        cmp     %esi, %eax
                        jb      .Ltopo_old
.Ltopo_ext:
                        mov     %esi, %eax
                        xor     %ecx, %ecx
                        cpuid
                        test    %ebx, %ebx
                        jz      .Ltopo_old
                        mov     %edx, %edi
                        jmp     .Ltopo_end
.Ltopo_old:
                        mov     $0x1, %eax
                        cpuid
                        shr     $0x18, %ebx
                        mov     %ebx, %edi
.Ltopo_end:
                        call    kern_ptab_setup
                        mov     %rax, %cr3
                        lea     DSTK_TOP, %rsp
                        jmp     bootstrap

.data

                        .balign 8
__relo_jip:             .long   0                               // Jump IP
__relo_jcs:             .word   0                               // Jump CS
__relo_pdl:             .word   .Lboot_gdt - __boot_gdt - 1     // Pseudo Descriptor Limit
__relo_pdb:             .quad   __boot_gdt                      // Pseudo Descriptor Base

.section .init

/*
 * Multiboot Header
 */
                        .balign 8
                        .long   0x1badb002
                        .long   BIT (0)
                        .long  -0x1badb002 - BIT (0)
