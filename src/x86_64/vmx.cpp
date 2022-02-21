/*
 * Virtual Machine Extensions (VMX)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "acpi.hpp"
#include "arch.hpp"
#include "bits.hpp"
#include "cmdline.hpp"
#include "cr.hpp"
#include "gdt.hpp"
#include "hip.hpp"
#include "idt.hpp"
#include "msr.hpp"
#include "ptab_ept.hpp"
#include "stdio.hpp"
#include "tss.hpp"
#include "util.hpp"
#include "vmx.hpp"

Vmcs *      Vmcs::root        { nullptr };
Vmcs *      Vmcs::current     { nullptr };
uint64_t    Vmcs::basic       { 0 };
uint64_t    Vmcs::ept_vpid    { 0 };
uint32_t    Vmcs::pin         { 0 };
uint32_t    Vmcs::ent         { 0 };
uint32_t    Vmcs::exi_pri     { 0 };
uint64_t    Vmcs::exi_sec     { 0 };
uint32_t    Vmcs::cpu_pri_clr { 0 }, Vmcs::cpu_pri_set { 0 };
uint32_t    Vmcs::cpu_sec_clr { 0 }, Vmcs::cpu_sec_set { 0 };
uint64_t    Vmcs::cpu_ter_clr { 0 }, Vmcs::cpu_ter_set { 0 };
uintptr_t   Vmcs::fix_cr0_clr { 0 }, Vmcs::fix_cr0_set { 0 };
uintptr_t   Vmcs::fix_cr4_clr { 0 }, Vmcs::fix_cr4_set { 0 };

void Vmcs::init (uintptr_t gsp, uintptr_t hsp, uintptr_t cr3, uint64_t apic, uint16_t vpid)
{
    // Set VMCS launch state to "clear" and initialize implementation-specific VMCS state.
    clear();

    make_current();

    write (Encoding::GUEST_RSP, gsp);

    write (Encoding::HOST_RSP, hsp);
    write (Encoding::HOST_RIP, reinterpret_cast<uintptr_t>(&entry_vmx));

    write (Encoding::HOST_CR3, cr3);
    write (Encoding::HOST_CR0, Cr::get_cr0() | CR0_TS);
    write (Encoding::HOST_CR4, Cr::get_cr4());

    write (Encoding::HOST_BASE_FS, 0);
    write (Encoding::HOST_BASE_GS, 0);
    write (Encoding::HOST_BASE_TR,   reinterpret_cast<uintptr_t>(&Tss::run));
    write (Encoding::HOST_BASE_GDTR, reinterpret_cast<uintptr_t>(Gdt::gdt));
    write (Encoding::HOST_BASE_IDTR, reinterpret_cast<uintptr_t>(Idt::idt));

    write (Encoding::HOST_SYSENTER_CS, 0);
    write (Encoding::HOST_SYSENTER_ESP, 0);
    write (Encoding::HOST_SYSENTER_EIP, 0);
    write (Encoding::HOST_PERF_GLOBAL_CTRL, 0);

    write (Encoding::PF_ERROR_MASK, 0);
    write (Encoding::PF_ERROR_MATCH, 0);
    write (Encoding::CR3_TARGET_COUNT, 0);

    write (Encoding::APIC_PAGE_ADDR, apic);
    write (Encoding::APIC_ACCS_ADDR, Kmem::ptr_to_phys (this));
    write (Encoding::VMCS_LINK_PTR, ~0ULL);
    write (Encoding::VPID, vpid);

    write (Encoding::HOST_SEL_CS, SEL_KERN_CODE);
    write (Encoding::HOST_SEL_SS, SEL_KERN_DATA);
    write (Encoding::HOST_SEL_DS, 0);
    write (Encoding::HOST_SEL_ES, 0);
    write (Encoding::HOST_SEL_FS, 0);
    write (Encoding::HOST_SEL_GS, 0);
    write (Encoding::HOST_SEL_TR, SEL_TSS_RUN);

    if (Cpu::feature (Cpu::Feature::PAT))
        write (Encoding::HOST_PAT, Msr::read (Msr::Reg64::IA32_PAT));

    if (Cpu::feature (Cpu::Feature::LM))
        write (Encoding::HOST_EFER, Msr::read (Msr::Reg64::IA32_EFER));

    if (Cpu::feature (Cpu::Feature::CET_SS) || Cpu::feature (Cpu::Feature::CET_IBT))
        write (Encoding::HOST_S_CET, Msr::read (Msr::Reg64::IA32_S_CET));

    if (Cpu::feature (Cpu::Feature::CET_SS)) {
        write (Encoding::HOST_SSP, reinterpret_cast<uintptr_t>(&SSTK_TOP));
        write (Encoding::HOST_SSP_TABLE_ADDR, Msr::read (Msr::Reg64::IA32_INTERRUPT_SSP_TABLE_ADDR));
    }

    write (Encoding::PIN_CONTROLS, pin);
    write (Encoding::ENT_CONTROLS, ent);
    write (Encoding::EXI_CONTROLS_PRI, exi_pri);
    write (Encoding::EXI_CONTROLS_SEC, exi_sec);

    write (Encoding::EXI_MSR_ST_CNT, 0);
    write (Encoding::EXI_MSR_LD_CNT, 0);
    write (Encoding::ENT_MSR_LD_CNT, 0);
}

void Vmcs::init()
{
    if (!Cpu::feature (Cpu::Feature::VMX) || (Msr::read (Msr::Reg64::IA32_FEATURE_CONTROL) & 0x5) != 0x5)
        return;

    if (!Acpi::resume) {

        // Basic VMX Information
        bool const ctrl = (basic = Msr::read (Msr::Reg64::IA32_VMX_BASIC)) & BIT64 (55);

        // Pin-Based Controls
        constexpr auto hyp_pin { Pin::PIN_VIRT_NMI | Pin::PIN_NMI | Pin::PIN_EXTINT };
        auto const vmx_pin { Msr::read (ctrl ? Msr::Reg64::IA32_VMX_TRUE_PIN : Msr::Reg64::IA32_VMX_CTRL_PIN) };
        pin = (hyp_pin | static_cast<uint32_t>(vmx_pin)) & static_cast<uint32_t>(vmx_pin >> 32);

        // VM-Entry Controls
        constexpr auto hyp_ent { Ent::ENT_LOAD_CET | Ent::ENT_LOAD_EFER | Ent::ENT_LOAD_PAT };
        auto const vmx_ent { Msr::read (ctrl ? Msr::Reg64::IA32_VMX_TRUE_ENT : Msr::Reg64::IA32_VMX_CTRL_ENT) };
        ent = (hyp_ent | static_cast<uint32_t>(vmx_ent)) & static_cast<uint32_t>(vmx_ent >> 32);

        // Primary VM-Exit Controls
        constexpr auto hyp_exi_pri { Exi_pri::EXI_SECONDARY | Exi_pri::EXI_LOAD_CET | Exi_pri::EXI_LOAD_EFER | Exi_pri::EXI_SAVE_EFER | Exi_pri::EXI_LOAD_PAT | Exi_pri::EXI_SAVE_PAT | Exi_pri::EXI_INTA | Exi_pri::EXI_HOST_64 };
        auto const vmx_exi_pri { Msr::read (ctrl ? Msr::Reg64::IA32_VMX_TRUE_EXI : Msr::Reg64::IA32_VMX_CTRL_EXI_PRI) };
        exi_pri = (hyp_exi_pri | static_cast<uint32_t>(vmx_exi_pri)) & static_cast<uint32_t>(vmx_exi_pri >> 32);

        // Secondary VM-Exit Controls
        constexpr auto hyp_exi_sec { 0 };
        auto const vmx_exi_sec { has_exi_sec() ? Msr::read (Msr::Reg64::IA32_VMX_CTRL_EXI_SEC) : 0 };
        exi_sec = hyp_exi_sec & vmx_exi_sec;

        // Primary VM-Execution Controls
        constexpr auto hyp_cpu_pri_clr { 0 };
        constexpr auto hyp_cpu_pri_set { Cpu_pri::CPU_SECONDARY | Cpu_pri::CPU_MSR_BITMAP | Cpu_pri::CPU_IO_BITMAP | Cpu_pri::CPU_IO | Cpu_pri::CPU_TERTIARY | Cpu_pri::CPU_HLT };
        auto const vmx_cpu_pri { Msr::read (ctrl ? Msr::Reg64::IA32_VMX_TRUE_CPU : Msr::Reg64::IA32_VMX_CTRL_CPU_PRI) };
        cpu_pri_clr = ~hyp_cpu_pri_clr & static_cast<uint32_t>(vmx_cpu_pri >> 32);
        cpu_pri_set =  hyp_cpu_pri_set | static_cast<uint32_t>(vmx_cpu_pri);

        // Secondary VM-Execution Controls
        constexpr auto hyp_cpu_sec_clr { Cpu_sec::CPU_INST_TIMEOUT | Cpu_sec::CPU_BUS_LOCK | Cpu_sec::CPU_EPC_VIRT | Cpu_sec::CPU_ENCLV | Cpu_sec::CPU_TSC_SCALING | Cpu_sec::CPU_SPP | Cpu_sec::CPU_PASID | Cpu_sec::CPU_PML | Cpu_sec::CPU_ENCLS | Cpu_sec::CPU_VMCS_SHADOW | Cpu_sec::CPU_VMFUNC | Cpu_sec::CPU_VIRT_INTR | Cpu_sec::CPU_VIRT_X2APIC };
        constexpr auto hyp_cpu_sec_set { Cpu_sec::CPU_MBEC | Cpu_sec::CPU_URG | Cpu_sec::CPU_VPID | Cpu_sec::CPU_EPT };
        auto const vmx_cpu_sec { has_cpu_sec() ? Msr::read (Msr::Reg64::IA32_VMX_CTRL_CPU_SEC) : 0 };
        cpu_sec_clr = ~hyp_cpu_sec_clr & static_cast<uint32_t>(vmx_cpu_sec >> 32);
        cpu_sec_set =  hyp_cpu_sec_set | static_cast<uint32_t>(vmx_cpu_sec);

        // Tertiary VM-Execution Controls
        constexpr auto hyp_cpu_ter_clr { Cpu_ter::CPU_PBNDKB | Cpu_ter::CPU_SPEC_CTRL | Cpu_ter::CPU_MSR_LIST | Cpu_ter::CPU_GPAW | Cpu_ter::CPU_IPI_VIRT | Cpu_ter::CPU_EPT_VPW | Cpu_ter::CPU_EPT_PW | Cpu_ter::CPU_HLAT | Cpu_ter::CPU_LOADIWKEY };
        constexpr auto hyp_cpu_ter_set { 0 };
        auto const vmx_cpu_ter { has_cpu_ter() ? Msr::read (Msr::Reg64::IA32_VMX_CTRL_CPU_TER) : 0 };
        cpu_ter_clr = ~hyp_cpu_ter_clr & vmx_cpu_ter;
        cpu_ter_set =  hyp_cpu_ter_set;

        // EPT and URG are mandatory
        if (!has_ept() || !has_urg())
            return;

        // EPT and VPID Capabilities
        ept_vpid = Msr::read (Msr::Reg64::IA32_VMX_EPT_VPID);

        // INVEPT is mandatory
        if (!has_invept())
            return;

        // INVVPID is optional and can be disabled
        if (!has_invvpid() || Cmdline::novpid)
            cpu_sec_clr &= ~Cpu_sec::CPU_VPID;

        // If the APIC timer stops in deep C-states, then force MWAIT exits
        if (!Cpu::feature (Cpu::Feature::ARAT))
            cpu_pri_set |= Cpu_pri::CPU_MWAIT;

        // MBEC is optional
        if (!has_mbec())
            Ept::mbec = false;

        // EPT maximum leaf level: 1 + { 1 (1GB), 0 (2MB), -1 (4KB) }
        Eptp::set_mll (1 + bit_scan_reverse (ept_vpid >> 16 & BIT_RANGE (1, 0)));

        fix_cr0_set =  Msr::read (Msr::Reg64::IA32_VMX_CR0_FIXED0);
        fix_cr0_clr = ~Msr::read (Msr::Reg64::IA32_VMX_CR0_FIXED1);
        fix_cr4_set =  Msr::read (Msr::Reg64::IA32_VMX_CR4_FIXED0);
        fix_cr4_clr = ~Msr::read (Msr::Reg64::IA32_VMX_CR4_FIXED1);

        fix_cr0_set &= ~(CR0_PG | CR0_PE);
        fix_cr0_clr |=   CR0_CD | CR0_NW;

        if (!(root = new Vmcs))
            return;

        Hip::set_feature (Hip_arch::Feature::VMX);
    }

    Cr::set_cr0 ((Cr::get_cr0() & ~fix_cr0_clr) | fix_cr0_set);
    Cr::set_cr4 ((Cr::get_cr4() & ~fix_cr4_clr) | fix_cr4_set);

    vmxon();

    trace (TRACE_VIRT, "VMCS: Revision:%#x (%#x:%#x:%#lx)", root->rev, cpu_pri_clr, cpu_sec_clr, cpu_ter_clr);
}

void Vmcs::fini()
{
    // FIXME: Must clear every VMCS on this core
    if (current)
        current->clear();

    if (root)
        vmxoff();
}
