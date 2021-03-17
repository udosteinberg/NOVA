/*
 * Secure Virtual Machine (SVM)
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

#pragma once

#include "kmem.hpp"
#include "utcb.hpp"

class Vmcb final
{
    public:
        union {
            char pad[1024];
            struct {
                uint32      intercept_cr;           // 0x0
                uint32      intercept_dr;           // 0x4
                uint32      intercept_exc;          // 0x8
                uint32      intercept_cpu[3];       // 0xc
                uint32      reserved1[10];          // 0x18
                uint64      base_io;                // 0x40
                uint64      base_msr;               // 0x48
                uint64      tsc_offset;             // 0x50
                uint32      asid;                   // 0x58
                uint32      tlb_control;            // 0x5c
                uint64      int_control;            // 0x60
                uint64      int_shadow;             // 0x68
                uint64      exitcode;               // 0x70
                uint64      exitinfo1;              // 0x78
                uint64      exitinfo2;              // 0x80
                uint64      exitintinfo;            // 0x88
                uint64      npt_control;            // 0x90
                uint32      reserved2[4];           // 0x98
                uint64      inj_control;            // 0xa8
                uint64      npt_cr3;                // 0xb0
                uint64      lbr;                    // 0xb8
            };
        };

        Utcb_segment        es, cs, ss, ds, fs, gs, gdtr, ldtr, idtr, tr;
        char                reserved3[48];
        uint64              efer;
        char                reserved4[112];
        uint64              cr4, cr3, cr0, dr7, dr6, rflags, rip;
        char                reserved5[88];
        uint64              rsp;
        char                reserved6[24];
        uint64              rax, star, lstar, cstar, sfmask, kernel_gs_base;
        uint64              sysenter_cs, sysenter_esp, sysenter_eip, cr2, nrip;
        char                reserved7[24];
        uint64              g_pat;

        static Paddr        root        CPULOCAL;
        static unsigned     asid_ctr    CPULOCAL;
        static uint32       svm_version CPULOCAL;
        static uint32       svm_feature CPULOCAL;

        static constexpr uintptr_t fix_cr0_set { 0 };
        static constexpr uintptr_t fix_cr0_clr { 0 };
        static constexpr uintptr_t fix_cr4_set { 0 };
        static constexpr uintptr_t fix_cr4_clr { 0 };

        enum Ctrl0
        {
            CPU_INTR        = BIT  (0),
            CPU_NMI         = BIT  (1),
            CPU_SMI         = BIT  (2),
            CPU_INIT        = BIT  (3),
            CPU_VINTR       = BIT  (4),
            CPU_W_CR0       = BIT  (5),
            CPU_R_IDTR      = BIT  (6),
            CPU_R_GDTR      = BIT  (7),
            CPU_R_LDTR      = BIT  (8),
            CPU_R_TR        = BIT  (9),
            CPU_W_IDTR      = BIT (10),
            CPU_W_GDTR      = BIT (11),
            CPU_W_LDTR      = BIT (12),
            CPU_W_TR        = BIT (13),
            CPU_RDTSC       = BIT (14),
            CPU_RDPMC       = BIT (15),
            CPU_PUSHF       = BIT (16),
            CPU_POPF        = BIT (17),
            CPU_CPUID       = BIT (18),
            CPU_RSM         = BIT (19),
            CPU_IRET        = BIT (20),
            CPU_INTN        = BIT (21),
            CPU_INVD        = BIT (22),
            CPU_PAUSE       = BIT (23),
            CPU_HLT         = BIT (24),
            CPU_INVLPG      = BIT (25),
            CPU_INVLPGA     = BIT (26),
            CPU_IO          = BIT (27),
            CPU_MSR         = BIT (28),
            CPU_TASK_SWITCH = BIT (29),
            CPU_FERR_FREEZE = BIT (30),
            CPU_SHUTDOWN    = BIT (31),
        };

        enum Ctrl1
        {
            CPU_VMRUN       = BIT  (0),
            CPU_VMMCALL     = BIT  (1),
            CPU_VMLOAD      = BIT  (2),
            CPU_VMSAVE      = BIT  (3),
            CPU_STGI        = BIT  (4),
            CPU_CLGI        = BIT  (5),
            CPU_SKINIT      = BIT  (6),
            CPU_RDTSCP      = BIT  (7),
            CPU_ICEBP       = BIT  (8),
            CPU_WBINVD      = BIT  (9),
            CPU_MONITOR     = BIT (10),
            CPU_MWAIT       = BIT (11),
            CPU_MWAIT_ARMED = BIT (12),
            CPU_XSETBV      = BIT (13),
            CPU_RDPRU       = BIT (14),
            CPU_EFER        = BIT (15),
        };

        enum Ctrl2
        {
            CPU_INVLPGB     = BIT  (0),
            CPU_INVLPGB_INV = BIT  (1),
            CPU_PCID        = BIT  (2),
            CPU_MCOMMIT     = BIT  (3),
            CPU_TLBSYNC     = BIT  (4),
        };

        static constexpr uint32 force_ctrl0 {   CPU_INTR        |
                                                CPU_NMI         |
                                                CPU_INIT        |
                                                CPU_INVD        |
                                                CPU_HLT         |
                                                CPU_IO          |
                                                CPU_MSR         |
                                                CPU_SHUTDOWN    };

        static constexpr uint32 force_ctrl1 {   CPU_VMLOAD      |
                                                CPU_VMSAVE      |
                                                CPU_CLGI        |
                                                CPU_SKINIT      };

        Vmcb (mword, mword);

        ALWAYS_INLINE
        inline Vmcb()
        {
            asm volatile ("vmsave" : : "a" (Kmem::ptr_to_phys (this)) : "memory");
        }

        static bool has_npt() { return Vmcb::svm_feature & 1; }
        static bool has_urg() { return true; }

        static void init();

        [[nodiscard]] ALWAYS_INLINE
        static inline void *operator new (size_t) noexcept
        {
            return Buddy::alloc (0, Buddy::Fill::BITS0);
        }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                Buddy::free (ptr);
        }
};
