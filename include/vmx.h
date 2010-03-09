/*
 * Virtual Machine Extensions (VMX)
 *
 * Copyright (C) 2006-2010, Udo Steinberg <udo@hypervisor.org>
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

#include "assert.h"
#include "buddy.h"
#include "compiler.h"

class Vmcs
{
    public:
        uint32  rev;
        uint32  abort;

        static Vmcs *current CPULOCAL_HOT;

        static unsigned vpid_ctr CPULOCAL;

        static union vmx_basic {
            uint64      val;
            struct {
                uint32  revision;
                uint32  size        : 13,
                                    :  3,
                        width       :  1,
                        dual        :  1,
                        type        :  4,
                        insouts     :  1,
                        ctrl        :  1;
            };
        } basic CPULOCAL;

        static union vmx_ept_vpid {
            uint64      val;
            struct {
                uint32              : 16,
                        super       :  2,
                                    :  2,
                        invept      :  1,
                                    : 11;
                uint32  invvpid     :  1;
            };
        } ept_vpid CPULOCAL;

        static union vmx_ctrl_pin {
            uint64      val;
            struct {
                uint32  set, clr;
            };
        } ctrl_pin CPULOCAL;

        static union vmx_ctrl_cpu {
            uint64      val;
            struct {
                uint32  set, clr;
            };
        } ctrl_cpu[2] CPULOCAL;

        static union vmx_ctrl_exi {
            uint64      val;
            struct {
                uint32  set, clr;
            };
        } ctrl_exi CPULOCAL;

        static union vmx_ctrl_ent {
            uint64      val;
            struct {
                uint32  set, clr;
            };
        } ctrl_ent CPULOCAL;

        static struct vmx_fix_cr0 {
            mword   set, clr;
        } fix_cr0 CPULOCAL;

        static struct vmx_fix_cr4 {
            mword   set, clr;
        } fix_cr4 CPULOCAL;

        enum Encoding
        {
            // 16-Bit Control Fields
            VPID                    = 0x0000ul,

            // 16-Bit Guest State Fields
            GUEST_SEL_ES            = 0x0800ul,
            GUEST_SEL_CS            = 0x0802ul,
            GUEST_SEL_SS            = 0x0804ul,
            GUEST_SEL_DS            = 0x0806ul,
            GUEST_SEL_FS            = 0x0808ul,
            GUEST_SEL_GS            = 0x080aul,
            GUEST_SEL_LDTR          = 0x080cul,
            GUEST_SEL_TR            = 0x080eul,

            // 16-Bit Host State Fields
            HOST_SEL_ES             = 0x0c00ul,
            HOST_SEL_CS             = 0x0c02ul,
            HOST_SEL_SS             = 0x0c04ul,
            HOST_SEL_DS             = 0x0c06ul,
            HOST_SEL_FS             = 0x0c08ul,
            HOST_SEL_GS             = 0x0c0aul,
            HOST_SEL_TR             = 0x0c0cul,

            // 64-Bit Control Fields
            IO_BITMAP_A             = 0x2000ul,
            IO_BITMAP_B             = 0x2002ul,
            MSR_BITMAP              = 0x2004ul,
            EXI_MSR_ST_ADDR         = 0x2006ul,
            EXI_MSR_LD_ADDR         = 0x2008ul,
            ENT_MSR_LD_ADDR         = 0x200aul,
            VMCS_EXEC_PTR           = 0x200cul,
            TSC_OFFSET              = 0x2010ul,
            TSC_OFFSET_HI           = 0x2011ul,
            APIC_VIRT_ADDR          = 0x2012ul,
            APIC_ACCS_ADDR          = 0x2014ul,
            EPTP                    = 0x201aul,
            EPTP_HI                 = 0x201bul,

            INFO_PHYS_ADDR          = 0x2400ul,

            // 64-Bit Guest State
            VMCS_LINK_PTR           = 0x2800ul,
            VMCS_LINK_PTR_HI        = 0x2801ul,
            GUEST_DEBUGCTL          = 0x2802ul,
            GUEST_DEBUGCTL_HI       = 0x2803ul,
            GUEST_PERF_GLOBAL_CTRL  = 0x2808ul,

            // 64-Bit Host State
            HOST_PERF_GLOBAL_CTRL   = 0x2c04ul,

            // 32-Bit Control Fields
            PIN_EXEC_CTRL           = 0x4000ul,
            CPU_EXEC_CTRL0          = 0x4002ul,
            EXC_BITMAP              = 0x4004ul,
            PF_ERROR_MASK           = 0x4006ul,
            PF_ERROR_MATCH          = 0x4008ul,
            CR3_TARGET_COUNT        = 0x400aul,
            EXI_CONTROLS            = 0x400cul,
            EXI_MSR_ST_CNT          = 0x400eul,
            EXI_MSR_LD_CNT          = 0x4010ul,
            ENT_CONTROLS            = 0x4012ul,
            ENT_MSR_LD_CNT          = 0x4014ul,
            ENT_INTR_INFO           = 0x4016ul,
            ENT_INTR_ERROR          = 0x4018ul,
            ENT_INST_LEN            = 0x401aul,
            TPR_THRESHOLD           = 0x401cul,
            CPU_EXEC_CTRL1          = 0x401eul,

            // 32-Bit R/O Data Fields
            VMX_INST_ERROR          = 0x4400ul,
            EXI_REASON              = 0x4402ul,
            EXI_INTR_INFO           = 0x4404ul,
            EXI_INTR_ERROR          = 0x4406ul,
            IDT_VECT_INFO           = 0x4408ul,
            IDT_VECT_ERROR          = 0x440aul,
            EXI_INST_LEN            = 0x440cul,
            EXI_INST_INFO           = 0x440eul,

            // 32-Bit Guest State Fields
            GUEST_LIMIT_ES          = 0x4800ul,
            GUEST_LIMIT_CS          = 0x4802ul,
            GUEST_LIMIT_SS          = 0x4804ul,
            GUEST_LIMIT_DS          = 0x4806ul,
            GUEST_LIMIT_FS          = 0x4808ul,
            GUEST_LIMIT_GS          = 0x480aul,
            GUEST_LIMIT_LDTR        = 0x480cul,
            GUEST_LIMIT_TR          = 0x480eul,
            GUEST_LIMIT_GDTR        = 0x4810ul,
            GUEST_LIMIT_IDTR        = 0x4812ul,
            GUEST_AR_ES             = 0x4814ul,
            GUEST_AR_CS             = 0x4816ul,
            GUEST_AR_SS             = 0x4818ul,
            GUEST_AR_DS             = 0x481aul,
            GUEST_AR_FS             = 0x481cul,
            GUEST_AR_GS             = 0x481eul,
            GUEST_AR_LDTR           = 0x4820ul,
            GUEST_AR_TR             = 0x4822ul,
            GUEST_INTR_STATE        = 0x4824ul,
            GUEST_ACTV_STATE        = 0x4826ul,
            GUEST_SMBASE            = 0x4828ul,
            GUEST_SYSENTER_CS       = 0x482aul,

            // 32-Bit Host State Fields
            HOST_SYSENTER_CS        = 0x4c00ul,

            // Natural-Width Control Fields
            CR0_MASK                = 0x6000ul,
            CR4_MASK                = 0x6002ul,
            CR0_READ_SHADOW         = 0x6004ul,
            CR4_READ_SHADOW         = 0x6006ul,
            CR3_TARGET_0            = 0x6008ul,
            CR3_TARGET_1            = 0x600aul,
            CR3_TARGET_2            = 0x600cul,
            CR3_TARGET_3            = 0x600eul,

            // Natural-Width R/O Data Fields
            EXI_QUALIFICATION       = 0x6400ul,
            IO_RCX                  = 0x6402ul,
            IO_RSI                  = 0x6404ul,
            IO_RDI                  = 0x6406ul,
            IO_RIP                  = 0x6408ul,
            GUEST_LINEAR_ADDRESS    = 0x640aul,

            // Natural-Width Guest State Fields
            GUEST_CR0               = 0x6800ul,
            GUEST_CR3               = 0x6802ul,
            GUEST_CR4               = 0x6804ul,
            GUEST_BASE_ES           = 0x6806ul,
            GUEST_BASE_CS           = 0x6808ul,
            GUEST_BASE_SS           = 0x680aul,
            GUEST_BASE_DS           = 0x680cul,
            GUEST_BASE_FS           = 0x680eul,
            GUEST_BASE_GS           = 0x6810ul,
            GUEST_BASE_LDTR         = 0x6812ul,
            GUEST_BASE_TR           = 0x6814ul,
            GUEST_BASE_GDTR         = 0x6816ul,
            GUEST_BASE_IDTR         = 0x6818ul,
            GUEST_DR7               = 0x681aul,
            GUEST_RSP               = 0x681cul,
            GUEST_RIP               = 0x681eul,
            GUEST_RFLAGS            = 0x6820ul,
            GUEST_PENDING_DEBUG     = 0x6822ul,
            GUEST_SYSENTER_ESP      = 0x6824ul,
            GUEST_SYSENTER_EIP      = 0x6826ul,

            // Natural-Width Host State Fields
            HOST_CR0                = 0x6c00ul,
            HOST_CR3                = 0x6c02ul,
            HOST_CR4                = 0x6c04ul,
            HOST_BASE_FS            = 0x6c06ul,
            HOST_BASE_GS            = 0x6c08ul,
            HOST_BASE_TR            = 0x6c0aul,
            HOST_BASE_GDTR          = 0x6c0cul,
            HOST_BASE_IDTR          = 0x6c0eul,
            HOST_SYSENTER_ESP       = 0x6c10ul,
            HOST_SYSENTER_EIP       = 0x6c12ul,
            HOST_RSP                = 0x6c14ul,
            HOST_RIP                = 0x6c16ul
        };

        enum Ctrl_exi
        {
            EXI_INTA                = 1ul << 15
        };

        enum Ctrl_pin
        {
            PIN_EXTINT              = 1ul << 0,
            PIN_NMI                 = 1ul << 3
        };

        enum Ctrl0
        {
            CPU_INTR_WINDOW         = 1ul << 2,
            CPU_HLT                 = 1ul << 7,
            CPU_INVLPG              = 1ul << 9,
            CPU_CR3_LOAD            = 1ul << 15,
            CPU_CR3_STORE           = 1ul << 16,
            CPU_IO                  = 1ul << 24,
            CPU_IO_BITMAP           = 1ul << 25,
            CPU_SECONDARY           = 1ul << 31,
        };

        enum Ctrl1
        {
            CPU_EPT                 = 1ul << 1,
            CPU_VPID                = 1ul << 5
        };

        enum Reason
        {
            VMX_EXCEPTION           = 0,
            VMX_EXTINT              = 1,
            VMX_TRIPLE_FAULT        = 2,
            VMX_INIT                = 3,
            VMX_SIPI                = 4,
            VMX_SMI_IO              = 5,
            VMX_SMI_OTHER           = 6,
            VMX_INTR_WINDOW         = 7,
            VMX_NMI_WINDOW          = 8,
            VMX_TASK_SWITCH         = 9,
            VMX_CPUID               = 10,
            VMX_GETSEC              = 11,
            VMX_HLT                 = 12,
            VMX_INVD                = 13,
            VMX_INVLPG              = 14,
            VMX_RDPMC               = 15,
            VMX_RDTSC               = 16,
            VMX_RSM                 = 17,
            VMX_VMCALL              = 18,
            VMX_VMCLEAR             = 19,
            VMX_VMLAUNCH            = 20,
            VMX_VMPTRLD             = 21,
            VMX_VMPTRST             = 22,
            VMX_VMREAD              = 23,
            VMX_VMRESUME            = 24,
            VMX_VMWRITE             = 25,
            VMX_VMXOFF              = 26,
            VMX_VMXON               = 27,
            VMX_CR                  = 28,
            VMX_DR                  = 29,
            VMX_IO                  = 30,
            VMX_RDMSR               = 31,
            VMX_WRMSR               = 32,
            VMX_FAIL_STATE          = 33,
            VMX_FAIL_MSR            = 34,
            VMX_MWAIT               = 36,
            VMX_MTF                 = 37,
            VMX_MONITOR             = 39,
            VMX_PAUSE               = 40,
            VMX_FAIL_MCHECK         = 41,
            VMX_TPR_THRESHOLD       = 43,
            VMX_APIC_ACCESS         = 44,
            VMX_GDTR_IDTR           = 46,
            VMX_LDTR_TR             = 47,
            VMX_EPT_VIOLATION       = 48,
            VMX_EPT_MISCONFIG       = 49,
            VMX_INVEPT              = 50,
            VMX_PREEMPT             = 52,
            VMX_INVVPID             = 53,
            VMX_WBINVD              = 54,
            VMX_XSETBV              = 55
        };

        ALWAYS_INLINE
        static inline void *operator new (size_t)
        {
            return Buddy::allocator.alloc (0, Buddy::NOFILL);
        }

        Vmcs (mword, mword, mword, mword);

        ALWAYS_INLINE
        inline Vmcs() : rev (basic.revision)
        {
            uint64 phys = Buddy::ptr_to_phys (this);

            bool ret;
            asm volatile ("vmxon %1; seta %0" : "=q" (ret) : "m" (phys) : "cc");
            assert (ret);
        }

        ALWAYS_INLINE
        inline void clear()
        {
            uint64 phys = Buddy::ptr_to_phys (this);

            bool ret;
            asm volatile ("vmclear %1; seta %0" : "=q" (ret) : "m" (phys) : "cc");
            assert (ret);
        }

        ALWAYS_INLINE
        inline void make_current()
        {
            if (EXPECT_TRUE (current == this))
                return;

            uint64 phys = Buddy::ptr_to_phys (current = this);

            bool ret;
            asm volatile ("vmptrld %1; seta %0" : "=q" (ret) : "m" (phys) : "cc");
            assert (ret);
        }

        ALWAYS_INLINE
        static inline mword read (Encoding enc)
        {
            mword val;
            asm volatile ("vmread %1, %0" : "=rm" (val) : "r" (static_cast<mword>(enc)) : "cc");
            return val;
        }

        ALWAYS_INLINE
        static inline void write (Encoding enc, mword val)
        {
            asm volatile ("vmwrite %0, %1" : : "rm" (val), "r" (static_cast<mword>(enc)) : "cc");
        }

        ALWAYS_INLINE
        static inline void adjust_rip()
        {
            write (GUEST_RIP, read (GUEST_RIP) + read (EXI_INST_LEN));

            // Undo any STI or MOV SS blocking
            uint32 intr = static_cast<uint32>(read (GUEST_INTR_STATE));
            if (EXPECT_FALSE (intr & 3))
                write (GUEST_INTR_STATE, intr & ~3);
        }

        ALWAYS_INLINE
        static inline unsigned long vpid()
        {
            return has_vpid() ? read (VPID) : 0;
        }

        static bool has_secondary() { return ctrl_cpu[0].clr & CPU_SECONDARY; }
        static bool has_ept()       { return ctrl_cpu[1].clr & CPU_EPT; }
        static bool has_vpid()      { return ctrl_cpu[1].clr & CPU_VPID; }

        static void init();
};
