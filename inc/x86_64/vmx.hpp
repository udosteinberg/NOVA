/*
 * Virtual Machine Extensions (VMX)
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

#pragma once

#include "assert.hpp"
#include "buddy.hpp"
#include "kmem.hpp"
#include "macros.hpp"
#include "std.hpp"

class Vmcs final
{
    private:
        uint32_t const  rev         { static_cast<uint32_t>(basic) };
        uint32_t const  abort       { 0 };

        static Vmcs *   root        CPULOCAL;
        static uint64_t basic       CPULOCAL;
        static uint64_t ept_vpid    CPULOCAL;
        static uint32_t pin         CPULOCAL;
        static uint32_t ent         CPULOCAL;
        static uint32_t exi_pri     CPULOCAL;
        static uint64_t exi_sec     CPULOCAL;

        static inline void vmxon()
        {
            uint64_t const phys { Kmem::ptr_to_phys (root) };

            bool ret;
            asm volatile ("vmxon %1" : "=@cca" (ret) : "m" (phys) : "cc");
            assert (ret);
        }

        static inline void vmxoff()
        {
            bool ret;
            asm volatile ("vmxoff" : "=@cca" (ret) : : "cc");
            assert (ret);
        }

    public:
        static Vmcs *       current     CPULOCAL;
        static uint32_t     cpu_pri_clr CPULOCAL;
        static uint32_t     cpu_pri_set CPULOCAL;
        static uint32_t     cpu_sec_clr CPULOCAL;
        static uint32_t     cpu_sec_set CPULOCAL;
        static uint64_t     cpu_ter_clr CPULOCAL;
        static uint64_t     cpu_ter_set CPULOCAL;
        static uintptr_t    fix_cr0_clr CPULOCAL;
        static uintptr_t    fix_cr0_set CPULOCAL;
        static uintptr_t    fix_cr4_clr CPULOCAL;
        static uintptr_t    fix_cr4_set CPULOCAL;

        enum class Encoding : uintptr_t
        {
            // 16-Bit Control Fields
            VPID                    = 0x0000,
            POSTED_INT_NOTIFICATION = 0x0002,
            EPTP_INDEX              = 0x0004,
            HLAT_PREFIX_SIZE        = 0x0006,
            LAST_PID_PTR            = 0x0008,

            // 16-Bit Guest State Fields
            GUEST_SEL_ES            = 0x0800,
            GUEST_SEL_CS            = 0x0802,
            GUEST_SEL_SS            = 0x0804,
            GUEST_SEL_DS            = 0x0806,
            GUEST_SEL_FS            = 0x0808,
            GUEST_SEL_GS            = 0x080a,
            GUEST_SEL_LDTR          = 0x080c,
            GUEST_SEL_TR            = 0x080e,
            GUEST_INT_STATUS        = 0x0810,
            PML_INDEX               = 0x0812,
            GUEST_UINV              = 0x0814,

            // 16-Bit Host State Fields
            HOST_SEL_ES             = 0x0c00,
            HOST_SEL_CS             = 0x0c02,
            HOST_SEL_SS             = 0x0c04,
            HOST_SEL_DS             = 0x0c06,
            HOST_SEL_FS             = 0x0c08,
            HOST_SEL_GS             = 0x0c0a,
            HOST_SEL_TR             = 0x0c0c,

            // 64-Bit Control Fields
            BITMAP_IO_A             = 0x2000,
            BITMAP_IO_B             = 0x2002,
            BITMAP_MSR              = 0x2004,
            EXI_MSR_ST_ADDR         = 0x2006,
            EXI_MSR_LD_ADDR         = 0x2008,
            ENT_MSR_LD_ADDR         = 0x200a,
            EXEC_VMCS_PTR           = 0x200c,
            PML_ADDRESS             = 0x200e,
            TSC_OFFSET              = 0x2010,
            APIC_PAGE_ADDR          = 0x2012,
            APIC_ACCS_ADDR          = 0x2014,
            POSTED_INT_DESC_ADDR    = 0x2016,
            VMFUNC_CONTROLS         = 0x2018,
            EPTP                    = 0x201a,
            BITMAP_EOI0             = 0x201c,
            BITMAP_EOI1             = 0x201e,
            BITMAP_EOI2             = 0x2020,
            BITMAP_EOI3             = 0x2022,
            EPTP_LIST               = 0x2024,
            BITMAP_VMREAD           = 0x2026,
            BITMAP_VMWRITE          = 0x2028,
            VE_INFO_ADDRESS         = 0x202a,
            BITMAP_XSS              = 0x202c,
            BITMAP_ENCLS            = 0x202e,
            SPP_TABLE_PTR           = 0x2030,
            TSC_MULTIPLIER          = 0x2032,
            CPU_CONTROLS_TER        = 0x2034,
            BITMAP_ENCLV            = 0x2036,
            PASID_DIR_ADDR_LO       = 0x2038,
            PASID_DIR_ADDR_HI       = 0x203a,
            SHARED_EPTP             = 0x203c,
            BITMAP_PCONFIG          = 0x203e,
            HLATP                   = 0x2040,
            PID_PTR                 = 0x2042,
            EXI_CONTROLS_SEC        = 0x2044,
            SPEC_CTRL_MASK          = 0x204a,
            SPEC_CTRL_SHADOW        = 0x204c,
            INJ_EVENT_DATA          = 0x2052,

            // 64-Bit R/O Data Fields
            GUEST_PHYSICAL_ADDRESS  = 0x2400,
            ORG_EVENT_DATA          = 0x2404,

            // 64-Bit Guest State Fields
            VMCS_LINK_PTR           = 0x2800,
            GUEST_DEBUGCTL          = 0x2802,
            GUEST_PAT               = 0x2804,
            GUEST_EFER              = 0x2806,
            GUEST_PERF_GLOBAL_CTRL  = 0x2808,
            GUEST_PDPTE0            = 0x280a,
            GUEST_PDPTE1            = 0x280c,
            GUEST_PDPTE2            = 0x280e,
            GUEST_PDPTE3            = 0x2810,
            GUEST_BNDCFGS           = 0x2812,
            GUEST_RTIT_CTL          = 0x2814,
            GUEST_LBR_CTL           = 0x2816,
            GUEST_PKRS              = 0x2818,
            GUEST_FRED_CONFIG       = 0x281a,
            GUEST_FRED_RSP1         = 0x281c,
            GUEST_FRED_RSP2         = 0x281e,
            GUEST_FRED_RSP3         = 0x2820,
            GUEST_FRED_STKLVLS      = 0x2822,
            GUEST_FRED_SSP1         = 0x2824,
            GUEST_FRED_SSP2         = 0x2826,
            GUEST_FRED_SSP3         = 0x2828,
            GUEST_FMASK             = 0x282a,
            GUEST_PL0_SSP           = 0x282c,

            // 64-Bit Host State Fields
            HOST_PAT                = 0x2c00,
            HOST_EFER               = 0x2c02,
            HOST_PERF_GLOBAL_CTRL   = 0x2c04,
            HOST_PKRS               = 0x2c06,
            HOST_FRED_CONFIG        = 0x2c08,
            HOST_FRED_RSP1          = 0x2c0a,
            HOST_FRED_RSP2          = 0x2c0c,
            HOST_FRED_RSP3          = 0x2c0e,
            HOST_FRED_STKLVLS       = 0x2c10,
            HOST_FRED_SSP1          = 0x2c12,
            HOST_FRED_SSP2          = 0x2c14,
            HOST_FRED_SSP3          = 0x2c16,
            HOST_FMASK              = 0x2c18,

            // 32-Bit Control Fields
            PIN_CONTROLS            = 0x4000,
            CPU_CONTROLS_PRI        = 0x4002,
            BITMAP_EXC              = 0x4004,
            PF_ERROR_MASK           = 0x4006,
            PF_ERROR_MATCH          = 0x4008,
            CR3_TARGET_COUNT        = 0x400a,
            EXI_CONTROLS_PRI        = 0x400c,
            EXI_MSR_ST_CNT          = 0x400e,
            EXI_MSR_LD_CNT          = 0x4010,
            ENT_CONTROLS            = 0x4012,
            ENT_MSR_LD_CNT          = 0x4014,
            INJ_EVENT_IDENT         = 0x4016,
            INJ_EVENT_ERROR         = 0x4018,
            ENT_INST_LEN            = 0x401a,
            TPR_THRESHOLD           = 0x401c,
            CPU_CONTROLS_SEC        = 0x401e,
            PLE_GAP                 = 0x4020,
            PLE_WINDOW              = 0x4022,
            NOTIFY_WINDOW           = 0x4024,

            // 32-Bit R/O Data Fields
            VMX_INST_ERROR          = 0x4400,
            EXI_REASON              = 0x4402,
            EXI_EVENT_IDENT         = 0x4404,
            EXI_EVENT_ERROR         = 0x4406,
            ORG_EVENT_IDENT         = 0x4408,
            ORG_EVENT_ERROR         = 0x440a,
            EXI_INST_LEN            = 0x440c,
            EXI_INST_INFO           = 0x440e,

            // 32-Bit Guest State Fields
            GUEST_LIMIT_ES          = 0x4800,
            GUEST_LIMIT_CS          = 0x4802,
            GUEST_LIMIT_SS          = 0x4804,
            GUEST_LIMIT_DS          = 0x4806,
            GUEST_LIMIT_FS          = 0x4808,
            GUEST_LIMIT_GS          = 0x480a,
            GUEST_LIMIT_LDTR        = 0x480c,
            GUEST_LIMIT_TR          = 0x480e,
            GUEST_LIMIT_GDTR        = 0x4810,
            GUEST_LIMIT_IDTR        = 0x4812,
            GUEST_AR_ES             = 0x4814,
            GUEST_AR_CS             = 0x4816,
            GUEST_AR_SS             = 0x4818,
            GUEST_AR_DS             = 0x481a,
            GUEST_AR_FS             = 0x481c,
            GUEST_AR_GS             = 0x481e,
            GUEST_AR_LDTR           = 0x4820,
            GUEST_AR_TR             = 0x4822,
            GUEST_INTR_STATE        = 0x4824,
            GUEST_ACTV_STATE        = 0x4826,
            GUEST_SMBASE            = 0x4828,
            GUEST_SYSENTER_CS       = 0x482a,
            PREEMPTION_TIMER        = 0x482e,

            // 32-Bit Host State Fields
            HOST_SYSENTER_CS        = 0x4c00,

            // Natural-Width Control Fields
            CR0_MASK                = 0x6000,
            CR4_MASK                = 0x6002,
            CR0_READ_SHADOW         = 0x6004,
            CR4_READ_SHADOW         = 0x6006,
            CR3_TARGET_0            = 0x6008,
            CR3_TARGET_1            = 0x600a,
            CR3_TARGET_2            = 0x600c,
            CR3_TARGET_3            = 0x600e,

            // Natural-Width R/O Data Fields
            EXI_QUALIFICATION       = 0x6400,
            IO_RCX                  = 0x6402,
            IO_RSI                  = 0x6404,
            IO_RDI                  = 0x6406,
            IO_RIP                  = 0x6408,
            GUEST_LINEAR_ADDRESS    = 0x640a,

            // Natural-Width Guest State Fields
            GUEST_CR0               = 0x6800,
            GUEST_CR3               = 0x6802,
            GUEST_CR4               = 0x6804,
            GUEST_BASE_ES           = 0x6806,
            GUEST_BASE_CS           = 0x6808,
            GUEST_BASE_SS           = 0x680a,
            GUEST_BASE_DS           = 0x680c,
            GUEST_BASE_FS           = 0x680e,
            GUEST_BASE_GS           = 0x6810,
            GUEST_BASE_LDTR         = 0x6812,
            GUEST_BASE_TR           = 0x6814,
            GUEST_BASE_GDTR         = 0x6816,
            GUEST_BASE_IDTR         = 0x6818,
            GUEST_DR7               = 0x681a,
            GUEST_RSP               = 0x681c,
            GUEST_RIP               = 0x681e,
            GUEST_RFLAGS            = 0x6820,
            GUEST_PENDING_DEBUG     = 0x6822,
            GUEST_SYSENTER_ESP      = 0x6824,
            GUEST_SYSENTER_EIP      = 0x6826,
            GUEST_S_CET             = 0x6828,
            GUEST_SSP               = 0x682a,
            GUEST_SSP_TABLE_ADDR    = 0x682c,

            // Natural-Width Host State Fields
            HOST_CR0                = 0x6c00,
            HOST_CR3                = 0x6c02,
            HOST_CR4                = 0x6c04,
            HOST_BASE_FS            = 0x6c06,
            HOST_BASE_GS            = 0x6c08,
            HOST_BASE_TR            = 0x6c0a,
            HOST_BASE_GDTR          = 0x6c0c,
            HOST_BASE_IDTR          = 0x6c0e,
            HOST_SYSENTER_ESP       = 0x6c10,
            HOST_SYSENTER_EIP       = 0x6c12,
            HOST_RSP                = 0x6c14,
            HOST_RIP                = 0x6c16,
            HOST_S_CET              = 0x6c18,
            HOST_SSP                = 0x6c1a,
            HOST_SSP_TABLE_ADDR     = 0x6c1c,
        };

        // Pin-Based Controls
        enum Pin
        {
            PIN_EXTINT              = BIT  (0),
            PIN_HOST_INT_FLAG       = BIT  (1),
            PIN_INIT                = BIT  (2),
            PIN_NMI                 = BIT  (3),
            PIN_SIPI                = BIT  (4),
            PIN_VIRT_NMI            = BIT  (5),
            PIN_PREEMPTION_TMR      = BIT  (6),
            PIN_POSTED_INTR         = BIT  (7),
        };

        // VM-Entry Controls
        enum Ent
        {
            ENT_LOAD_CR0_CR4        = BIT  (0),
            ENT_LOAD_CR3            = BIT  (1),
            ENT_LOAD_DEBUG          = BIT  (2),
            ENT_LOAD_SEGMENTS       = BIT  (3),
            ENT_LOAD_RSP_RIP_RFL    = BIT  (4),
            ENT_LOAD_PENDING_DEBUG  = BIT  (5),
            ENT_LOAD_INTR_STATE     = BIT  (6),
            ENT_LOAD_ACTV_STATE     = BIT  (7),
            ENT_LOAD_WORKING_VMCS   = BIT  (8),
            ENT_GUEST_64            = BIT  (9),
            ENT_SMM                 = BIT (10),
            ENT_DEACTIVATE_DM       = BIT (11),
            ENT_LOAD_SYSENTER       = BIT (12),
            ENT_LOAD_PERF_GLOBAL    = BIT (13),
            ENT_LOAD_PAT            = BIT (14),
            ENT_LOAD_EFER           = BIT (15),
            ENT_LOAD_BNDCFGS        = BIT (16),
            ENT_RTIT_SUPPRESS_PIP   = BIT (17),
            ENT_LOAD_RTIT_CTL       = BIT (18),
            ENT_LOAD_UINV           = BIT (19),
            ENT_LOAD_CET            = BIT (20),
            ENT_LOAD_LBR_CTL        = BIT (21),
            ENT_LOAD_PKRS           = BIT (22),
            ENT_LOAD_FRED           = BIT (23),
        };

        // Primary VM-Exit Controls
        enum Exi_pri
        {
            EXI_SAVE_CR0_CR4        = BIT  (0),
            EXI_SAVE_CR3            = BIT  (1),
            EXI_SAVE_DEBUG          = BIT  (2),
            EXI_SAVE_SEGMENTS       = BIT  (3),
            EXI_SAVE_RSP_RIP_RFL    = BIT  (4),
            EXI_SAVE_PENDING_DEBUG  = BIT  (5),
            EXI_SAVE_INTR_STATE     = BIT  (6),
            EXI_SAVE_ACTV_STATE     = BIT  (7),
            EXI_SAVE_WORKING_VMCS   = BIT  (8),
            EXI_HOST_64             = BIT  (9),
            EXI_LOAD_CR0_CR4        = BIT (10),
            EXI_LOAD_CR3            = BIT (11),
            EXI_LOAD_PERF_GLOBAL    = BIT (12),
            EXI_LOAD_SEGMENTS       = BIT (13),
            EXI_LOAD_RSP_RIP        = BIT (14),
            EXI_INTA                = BIT (15),
            EXI_SAVE_SYSENTER       = BIT (16),
            EXI_LOAD_SYSENTER       = BIT (17),
            EXI_SAVE_PAT            = BIT (18),
            EXI_LOAD_PAT            = BIT (19),
            EXI_SAVE_EFER           = BIT (20),
            EXI_LOAD_EFER           = BIT (21),
            EXI_SAVE_PREEMPTION_TMR = BIT (22),
            EXI_CLEAR_BNDCFGS       = BIT (23),
            EXI_RTIT_SUPPRESS_PIP   = BIT (24),
            EXI_CLEAR_RTIT_CTL      = BIT (25),
            EXI_CLEAR_LBR_CTL       = BIT (26),
            EXI_CLEAR_UINV          = BIT (27),
            EXI_LOAD_CET            = BIT (28),
            EXI_LOAD_PKRS           = BIT (29),
            EXI_SAVE_PERF_GLOBAL    = BIT (30),
            EXI_SECONDARY           = BIT (31),
        };

        // Secondary VM-Exit Controls
        enum Exi_sec
        {
            EXI_SAVE_FRED           = BIT  (0),
            EXI_LOAD_FRED           = BIT  (1),
        };

        // Primary VM-Execution Controls
        enum Cpu_pri
        {
            CPU_INTR_WINDOW         = BIT  (2),
            CPU_TSC_OFFSETTING      = BIT  (3),
            CPU_HLT                 = BIT  (7),
            CPU_INVLPG              = BIT  (9),
            CPU_MWAIT               = BIT (10),
            CPU_RDPMC               = BIT (11),
            CPU_RDTSC               = BIT (12),
            CPU_CR3_LOAD            = BIT (15),
            CPU_CR3_STORE           = BIT (16),
            CPU_TERTIARY            = BIT (17),
            CPU_CR8_LOAD            = BIT (19),
            CPU_CR8_STORE           = BIT (20),
            CPU_TPR_SHADOW          = BIT (21),
            CPU_NMI_WINDOW          = BIT (22),
            CPU_DR                  = BIT (23),
            CPU_IO                  = BIT (24),
            CPU_IO_BITMAP           = BIT (25),
            CPU_MTF                 = BIT (27),
            CPU_MSR_BITMAP          = BIT (28),
            CPU_MONITOR             = BIT (29),
            CPU_PAUSE               = BIT (30),
            CPU_SECONDARY           = BIT (31),
        };

        // Secondary VM-Execution Controls
        enum Cpu_sec
        {
            CPU_VIRT_APIC_ACCS      = BIT  (0),
            CPU_EPT                 = BIT  (1),
            CPU_DESC_TABLE          = BIT  (2),
            CPU_RDTSCP              = BIT  (3),
            CPU_VIRT_X2APIC         = BIT  (4),
            CPU_VPID                = BIT  (5),
            CPU_WBINVD              = BIT  (6),
            CPU_URG                 = BIT  (7),
            CPU_VIRT_APIC_REGS      = BIT  (8),
            CPU_VIRT_INTR           = BIT  (9),
            CPU_PAUSE_LOOP          = BIT (10),
            CPU_RDRAND              = BIT (11),
            CPU_INVPCID             = BIT (12),
            CPU_VMFUNC              = BIT (13),
            CPU_VMCS_SHADOW         = BIT (14),
            CPU_ENCLS               = BIT (15),
            CPU_RDSEED              = BIT (16),
            CPU_PML                 = BIT (17),
            CPU_EPT_VE              = BIT (18),
            CPU_CONCEAL_NONROOT     = BIT (19),
            CPU_XSAVES_XRSTORS      = BIT (20),
            CPU_PASID               = BIT (21),
            CPU_MBEC                = BIT (22),
            CPU_SPP                 = BIT (23),
            CPU_PT2GPA              = BIT (24),
            CPU_TSC_SCALING         = BIT (25),
            CPU_UWAIT               = BIT (26),
            CPU_PCONFIG             = BIT (27),
            CPU_ENCLV               = BIT (28),
            CPU_EPC_VIRT            = BIT (29),
            CPU_BUS_LOCK            = BIT (30),
            CPU_NOTIFICATION        = BIT (31),
        };

        // Tertiary VM-Execution Controls
        enum Cpu_ter
        {
            CPU_LOADIWKEY           = BIT  (0),
            CPU_HLAT                = BIT  (1),
            CPU_EPT_PW              = BIT  (2),
            CPU_EPT_VPW             = BIT  (3),
            CPU_IPI_VIRT            = BIT  (4),
            CPU_GPAW                = BIT  (5),
            CPU_SPEC_CTRL           = BIT  (7),
        };

        // Basic Exit Reasons
        enum Reason
        {
            VMX_EXC_NMI             = 0,
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
            VMX_XSETBV              = 55,
        };

        void init (uintptr_t, uintptr_t, uintptr_t, uint64_t, uint16_t);

        ALWAYS_INLINE
        inline void clear()
        {
            if (EXPECT_FALSE (current == this))
                current = nullptr;

            uint64_t const phys { Kmem::ptr_to_phys (this) };

            bool ret;
            asm volatile ("vmclear %1" : "=@cca" (ret) : "m" (phys));
            assert (ret);
        }

        ALWAYS_INLINE
        inline void make_current()
        {
            if (EXPECT_TRUE (current == this))
                return;

            uint64_t const phys { Kmem::ptr_to_phys (current = this) };

            bool ret;
            asm volatile ("vmptrld %1" : "=@cca" (ret) : "m" (phys));
            assert (ret);
        }

        template <typename T>
        ALWAYS_INLINE
        static inline T read (Encoding e)
        {
            uintptr_t v;
            asm volatile ("vmread %1, %0" : "=rm" (v) : "r" (std::to_underlying (e)) : "cc");
            return static_cast<T>(v);
        }

        template <typename T>
        ALWAYS_INLINE
        static inline void write (Encoding e, T v)
        {
            asm volatile ("vmwrite %0, %1" : : "rm" (static_cast<uintptr_t>(v)), "r" (std::to_underlying (e)) : "cc");
        }

        ALWAYS_INLINE
        static inline uint16_t vpid()
        {
            return has_vpid() ? read<uint16_t> (Encoding::VPID) : 0;
        }

        static inline bool has_exi_sec()        { return exi_pri     & Exi_pri::EXI_SECONDARY; }
        static inline bool has_cpu_sec()        { return cpu_pri_clr & Cpu_pri::CPU_SECONDARY; }
        static inline bool has_cpu_ter()        { return cpu_pri_clr & Cpu_pri::CPU_TERTIARY; }
        static inline bool has_ept()            { return cpu_sec_clr & Cpu_sec::CPU_EPT; }
        static inline bool has_vpid()           { return cpu_sec_clr & Cpu_sec::CPU_VPID; }
        static inline bool has_urg()            { return cpu_sec_clr & Cpu_sec::CPU_URG; }
        static inline bool has_mbec()           { return cpu_sec_clr & Cpu_sec::CPU_MBEC; }
        static inline bool has_invept()         { return ept_vpid & BIT64 (20); }
        static inline bool has_invvpid()        { return ept_vpid & BIT64 (32); }
        static inline bool has_invvpid_sgl()    { return ept_vpid & BIT64 (41); }

        static void init();
        static void fini();

        [[nodiscard]] static void *operator new (size_t) noexcept
        {
            return Buddy::alloc (0, Buddy::Fill::NONE);
        }

        static void operator delete (void *ptr)
        {
            Buddy::free (ptr);
        }
};
