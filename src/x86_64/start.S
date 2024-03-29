/*
 * Startup Code
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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
#include "memattr.hpp"
#include "memory.hpp"
#include "multiboot.hpp"
#include "patch.hpp"
#include "selectors.hpp"

#define PTE_ATTR_P      BIT (0)     // Present
#define PTE_ATTR_W      BIT (1)     // Writable
#define PTE_ATTR_U      BIT (2)     // User
#define PTE_ATTR_A      BIT (5)     // Accessed
#define PTE_ATTR_D      BIT (6)     // Dirty
#define PTE_ATTR_S      BIT (7)     // Superpage
#define PTE_ATTR_G      BIT (8)     // Global

.globl                  __init_bsp, __init_aps, __init_aps__, __wake_vec, __init_ilp

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

.macro                  RECTIME SYMBOL
                        rdtsc
                        mov     %eax, 0 + \SYMBOL - OFFSET(%ebp)
                        mov     %edx, 4 + \SYMBOL - OFFSET(%ebp)
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

__init_bsp:             // Check if we launched via Multiboot v2
                        cmp     $MULTIBOOT_V2_LOADER, %eax
                        je      .Lvalid_launch

                        // Check if we launched via Multiboot v1
                        cmp     $MULTIBOOT_V1_LOADER, %eax
                        je      .Lvalid_launch

                        hlt
                        jmp     .
.Lvalid_launch:
                        // Determine relocation offset
                        xchg    %ebx, %esp
                        pop     %esi
                        call    .Lret_addr
.Lret_addr:             pop     %ebp
                        push    %esi
            dec %esi;   sub     $.Lret_addr, %ebp
                        xchg    %ebx, %esp

                        // Store boot parameters
                        mov     %eax, multiboot_p0 - OFFSET(%ebp)
                        mov     %ebx, multiboot_p1 - OFFSET(%ebp)

                        RECTIME multiboot_t0

                        // Store image end address
                        lea     NOVA_HPAE(%ebp), %eax
                        mov     %eax, multiboot_ea - OFFSET(%ebp)
.Lmb2_launch:
                        // Multiboot v2 Launch
                        cmpl    $MULTIBOOT_V2_LOADER, multiboot_p0 - OFFSET(%ebp)
                        jne     .Lmb1_launch

                        // Prepare tag loop
                        lea     8(%ebx), %ecx       // Tag Ptr
                        add     0(%ebx), %ebx       // Tag End
                        xor     %esi, %esi          // EFI Sys
                        xor     %edi, %edi          // EFI Img
.Lmb2_tag_loop:
                        cmpl    $MULTIBOOT_V2_INFO_END, (%ecx)
                        je      .Lmb2_end

                        cmpl    $MULTIBOOT_V2_INFO_CMD, (%ecx)
                        jne     .Lmb2_not_cmd
                        lea     8(%ecx), %eax
                        mov     %eax, multiboot_cl - OFFSET(%ebp)
                        jmp     .Lmb2_nxt_tag
.Lmb2_not_cmd:
                        cmpl    $MULTIBOOT_V2_INFO_MOD, (%ecx)
                        jne     .Lmb2_not_mod
                        cmpl    $0x0, multiboot_ra - OFFSET(%ebp)
                        jne     .Lmb2_nxt_tag
                        mov     8(%ecx), %eax
                        mov     %eax, multiboot_ra - OFFSET(%ebp)
                        jmp     .Lmb2_nxt_tag
.Lmb2_not_mod:
                        cmpl    $MULTIBOOT_V2_INFO_SYS, (%ecx)
                        jne     .Lmb2_not_sys
                        mov     8(%ecx), %esi
                        jmp     .Lmb2_nxt_tag
.Lmb2_not_sys:
                        cmpl    $MULTIBOOT_V2_INFO_IMG, (%ecx)
                        jne     .Lmb2_not_img
                        mov     8(%ecx), %edi
                        jmp     .Lmb2_nxt_tag
.Lmb2_not_img:
                        cmpl    $MULTIBOOT_V2_INFO_KMEM, (%ecx)
                        jne     .Lmb2_nxt_tag
                        mov     $0x20000000, %edx
                        sub     $(NOVA_HPAE - LOAD_ADDR), %edx
                        mov     8(%ecx), %eax
                        cmp     %edx, %eax
                        cmova   %edx, %eax
                        and     $~OFFS_MASK (1), %eax
                        add     %eax, multiboot_ea - OFFSET(%ebp)
.Lmb2_nxt_tag:
                        mov     4(%ecx), %eax
                        lea     7(%ecx,%eax), %ecx
                        and     $~7, %ecx
                        cmp     %ebx, %ecx
                        jb      .Lmb2_tag_loop
.Lmb2_end:
                        // Initialize UEFI
                        test    %edi, %edi
                        jz      .Lmb_end
                        test    %esi, %esi
                        jz      .Lmb_end
                        lea     uefi_info - OFFSET(%ebp), %edx
                        call    uefi_init
                        jmp     .Lmb_end
.Lmb1_launch:
                        // Multiboot v1 Launch
                        cmpl    $MULTIBOOT_V1_LOADER, multiboot_p0 - OFFSET(%ebp)
                        jne     .Lmb_end

                        testb   $BIT (MULTIBOOT_V1_INFO_CMD), (%ebx)
                        jz      .Lmb1_not_cmd
                        mov     16(%ebx), %eax
                        mov     %eax, multiboot_cl - OFFSET(%ebp)
.Lmb1_not_cmd:
                        testb   $BIT (MULTIBOOT_V1_INFO_MOD), (%ebx)
                        jz      .Lmb_end
                        mov     20(%ebx), %eax
                        test    %eax, %eax
                        jz      .Lmb_end
                        mov     24(%ebx), %eax
                        mov     (%eax), %eax
                        mov     %eax, multiboot_ra - OFFSET(%ebp)
.Lmb_end:
                        RECTIME multiboot_t1

                        // Ensure flags are in a defined state (after UEFI init)
                        cli
                        cld

                        // Load relocated boot GDT
                        REL_SYM (__relo_pdb - OFFSET)
                        lgdt    (__relo_pdl - OFFSET)(%ebp)

                        // Configure MLE Page Tables (4K pages)
                        PTE_FIX 2, MLE_L2, 0, MLE_L1, PTE_ATTR_P
                        PTE_FIX 1, MLE_L1, 0, MLE_L0, PTE_ATTR_P

                        lea     MLE_L0(%ebp), %edx
                        lea     HASH_HPAS + PTE_ATTR_P(%ebp), %ecx
                        lea     HASH_HPAE(%ebp), %eax
.Lmle_loop:             mov     %ecx, (%edx)
                        add     $__SIZEOF_POINTER__, %edx
                        add     $PAGE_SIZE (0), %ecx
                        cmp     %eax, %ecx
                        jb      .Lmle_loop
.Lilp:
                        // Configure Page Tables: Load Section (2M page)
                        PTE_REL 3, PT3S_HPAS, NOVA_HPAS, PT2L_HPAS, PTE_ATTR_A | PTE_ATTR_U | PTE_ATTR_W | PTE_ATTR_P
                        PTE_REL 2, PT2L_HPAS, NOVA_HPAS, PT1L_HPAS, PTE_ATTR_A | PTE_ATTR_U | PTE_ATTR_W | PTE_ATTR_P
                        PTE_REL 1, PT1L_HPAS, NOVA_HPAS, NOVA_HPAS, PTE_ATTR_S | PTE_ATTR_D | PTE_ATTR_A | PTE_ATTR_W | PTE_ATTR_P

                        // Configure Page Tables: Link Section (2M pages)
                        PTE_FIX 3, PT3S_HPAS, LINK_ADDR, PT2H_HPAS, PTE_ATTR_A | PTE_ATTR_U | PTE_ATTR_W | PTE_ATTR_P
                        PTE_FIX 2, PT2H_HPAS, LINK_ADDR, PT1H_HPAS, PTE_ATTR_A | PTE_ATTR_U | PTE_ATTR_W | PTE_ATTR_P

                        lea     PT1H_HPAS + __SIZEOF_POINTER__ * (LINK_ADDR >> (PTE_BPL + PAGE_BITS) & (BIT (PTE_BPL) - 1))(%ebp), %edx
                        lea     NOVA_HPAS + (PTE_ATTR_G | PTE_ATTR_S | PTE_ATTR_D | PTE_ATTR_A | PTE_ATTR_W | PTE_ATTR_P)(%ebp), %ecx
                        mov     multiboot_ea - OFFSET(%ebp), %eax
.Lpte_loop:             mov     %ecx, (%edx)
                        add     $__SIZEOF_POINTER__, %edx
                        add     $PAGE_SIZE (1), %ecx
                        cmp     %eax, %ecx
                        jb      .Lpte_loop

                        RECTIME multiboot_t2

                        // Initialize jump target
                        lea     __init_all(%ebp), %ebx
                        mov     %ebx,           __relo_jip - OFFSET(%ebp)
                        movw    $SEL_KERN_CODE, __relo_jcs - OFFSET(%ebp)
.Lrlp:
                        INIT_PAGING
                        ljmp   *__relo_jip - OFFSET(%ebp)

/*
 * ILP Re-Entry
 */
__init_ilp:             // Determine load skew
                        mov     %ebx, %ebp
                        sub     $__init_ilp, %ebp

                        // Crash if load skew is misaligned
                        test    $OFFS_MASK (1), %ebp
                        jnz     .Lcrash

                        // Crash if pseudo descriptor limit is wrong
                        mov     $.Lboot_gdt - __boot_gdt - 1, %eax
                        cmp     %ax, %cs:__relo_pdl - OFFSET(%ebp)
                        jne     .Lcrash

                        // Crash if pseudo descriptor base is wrong
                        lea     __boot_gdt(%ebp), %ebx
                        cmp     %ebx, %cs:__relo_pdb - OFFSET(%ebp)
                        jne     .Lcrash

                        // Load measured boot GDT
                        lgdt    %cs:__relo_pdl - OFFSET(%ebp)

                        // Establish valid 32-bit segments for SS/DS/ES
                        mov     $SEL_KERN_DATA + 16, %ecx
                        mov     %cx, %ss
                        mov     %cx, %ds
                        mov     %cx, %es

                        // Initialize MLE join structure
                        sub     $8, %ecx
                        lea     __init_rlp(%ebp), %edx
                        lea     mle_join - OFFSET(%ebp), %edi
                        mov     %eax,  0(%edi)                      // GDT Limit
                        mov     %ebx,  4(%edi)                      // GDT Base
                        mov     %ecx,  8(%edi)                      // SEL
                        mov     %edx, 12(%edi)                      // EIP

                        // Traverse TXT heap
                        mov     0xfed30300, %esi                    // Data_bios
                        add     (%esi), %esi                        // Data_os_mle
                        add     (%esi), %esi                        // Data_os_sinit

                        // Crash if Data_os_sinit does not point to MLE_L2
                        lea     MLE_L2(%ebp), %eax
                        cmp     %eax, 16(%esi)
                        jne     .Lcrash

                        // Crash if MLE_L2 does not point to MLE_L1 + attributes
                        lea     MLE_L1 + PTE_ATTR_P(%ebp), %eax
                        cmp     %eax, MLE_L2(%ebp)
                        jne     .Lcrash

                        // Crash if MLE_L1 does not point to MLE_L0 + attributes
                        lea     MLE_L0 + PTE_ATTR_P(%ebp), %eax
                        cmp     %eax, MLE_L1(%ebp)
                        jne     .Lcrash

                        // Crash if MLE first page is not virt/phys contiguous with re-entry point
                        lea     HASH_HPAS + PTE_ATTR_P(%ebp), %eax
                        cmp     %eax, MLE_L0(%ebp)
                        jne     .Lcrash

                        // Crash if MLE last page is not virt/phys contiguous with re-entry point
                        lea     HASH_HPAL + PTE_ATTR_P(%ebp), %eax
                        cmp     %eax, MLE_L0_LS(%ebp)
                        jne     .Lcrash

                        // Determine image range
                        lea     NOVA_HPAS(%ebp), %eax               // NOVA.S
                        lea     NOVA_HPAE(%ebp), %ecx               // NOVA.E (original)
                        mov     multiboot_ea - OFFSET(%ebp), %ebx   // NOVA.E (extended)

                        // Crash if NOVA.E is wrong (extended < original)
                        cmp     %ecx, %ebx
                        jb      .Lcrash

                        // Crash if NOVA.E is misaligned
                        test    $OFFS_MASK (1), %ebx
                        jnz     .Lcrash

                        // Crash if PMR does not cover NOVA.S...NOVA.E
                        mov     %ebx, %ecx
                        sub     %eax, %ecx
                        cmp     %eax, 40(%esi)                      // PMR Lo Base
                        jne     .Lcrash
                        cmp     %ecx, 48(%esi)                      // PMR Lo Size
                        jne     .Lcrash

                        // Traverse TXT heap
                        add     (%esi), %esi                        // Data_sinit_mle

                        // Crash if Data_sinit_mle version < 5
                        cmpl    $5, 8(%esi)
                        jb      .Lcrash

                        // Traverse MDRs
                        mov     %esi, %edi
                        mov     136(%esi), %ecx                     // MDR count
                        add     140(%esi), %edi                     // MDR pointer
.Lmdr_loop:
                        // Check if NOVA.S (ESP:EAX) >= MDR.S (ESI:EDX)
                        mov     0(%edi), %edx
                        mov     4(%edi), %esi
                        xor     %esp, %esp
                        cmp     %edx, %eax
                        sbb     %esi, %esp
                        jc      .Lmdr_next                          // NOVA.S < MDR.S

                        // Check if NOVA.E (ESP:EBX) <= MDR.E (ESI:EDX)
                        add      8(%edi), %edx
                        adc     12(%edi), %esi
                        xor     %esp, %esp
                        cmp     %ebx, %edx
                        sbb     %esp, %esi
                        jc      .Lmdr_next                          // NOVA.E > MDR.E

                        // Crash if MDR covering NOVA is != type 0
                        cmpb    $0, 16(%edi)
                        jne     .Lcrash

                        // All post-launch checks succeeded
                        orb     $1, launched - OFFSET(%ebp)
                        jmp     .Lilp
.Lmdr_next:
                        lea     24(%edi), %edi
                        loop    .Lmdr_loop
.Lcrash:
                        ud2

/*
 * RLP Re-Entry
 */
__init_rlp:             // Determine load skew
                        mov     0xfed30290, %ebp
                        sub     $mle_join - OFFSET, %ebp

                        // Mark as RLP
                        xor     %ebx, %ebx

                        jmp     .Lrlp

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
                        .quad   0x00cf9b000000ffff              // SEL_KERN_CODE (32-bit)
                        .quad   0x00cf93000000ffff              // SEL_KERN_DATA (32-bit)
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

                        call    preinit
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
                        // Restore TXT state
                        call    txt_restore

                        // Init (BSP only)
                        test    %ebx, %ebx
                        jz      .Lskip_init

                        // Apply these relocations post-launch
                        REL_SYM .Lrelo_jip
                        REL_SYM .Lrelo_pdb

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
#if defined(__CET__)
                        // Track CET features in EAX
                        xor     %eax, %eax
#if (__CET__ & 1)
                        #define ASM_ENABLE_CET_1 or $(BIT_RANGE (5, 4) | BIT (2)), %al
                        PATCH  (ASM_ENABLE_CET_1,, PATCH_CET_IBT);
#endif
#if (__CET__ & 2)
                        #define ASM_ENABLE_CET_2 or $BIT (0), %al
                        PATCH  (ASM_ENABLE_CET_2,, PATCH_CET_SSS);
#endif
                        // Check if any CET features are being enabled
                        test    %eax, %eax
                        jz      .Lcet_end

                        // Enable CET
                        mov     %cr4, %rdx
                        or      $CR4_CET, %edx
                        mov     %rdx, %cr4

                        // Enable CET features in EAX
                        mov     $0x6a2, %ecx
                        xor     %edx, %edx
                        wrmsr
#if (__CET__ & 2)
                        // Check if CET_SSS is enabled
                        test    $BIT (0), %al
                        jz      .Lcet_end

                        // Activate supervisor shadow stack
                        mov     $0x6a4, %ecx
                        lea     SSTK_TOP, %rdx
                        mov     %rdx, %rax
                        shr     $0x20, %rdx
                        wrmsr
                        setssbsy
#endif
.Lcet_end:
#endif
                        jmp     bootstrap

.data

                        .balign 8
__relo_jip:             .long   0                               // Jump IP
__relo_jcs:             .word   0                               // Jump CS
__relo_pdl:             .word   .Lboot_gdt - __boot_gdt - 1     // Pseudo Descriptor Limit
__relo_pdb:             .quad   __boot_gdt                      // Pseudo Descriptor Base
