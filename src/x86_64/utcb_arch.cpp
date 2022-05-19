/*
 * User Thread Control Block (UTCB)
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

#include "config.hpp"
#include "event.hpp"
#include "regs.hpp"
#include "space_gst.hpp"
#include "space_msr.hpp"
#include "space_obj.hpp"
#include "space_pio.hpp"
#include "svm.hpp"
#include "vmx.hpp"
#include "vpid.hpp"

void Utcb_arch::load_exc (Mtd_arch const m, Exc_regs const &e)
{
    auto const &s { e.sys };

    if (m & Mtd_arch::Item::GPR_0_7) {
        rax = s.rax; rcx = s.rcx; rdx = s.rdx; rbx = s.rbx;
        rsp = e.rsp; rbp = s.rbp; rsi = s.rsi; rdi = s.rdi;
    }

    if (m & Mtd_arch::Item::GPR_8_15) {
        r8  = s.r8;  r9  = s.r9;  r10 = s.r10; r11 = s.r11;
        r12 = s.r12; r13 = s.r13; r14 = s.r14; r15 = s.r15;
    }

    if (m & Mtd_arch::Item::RFLAGS)
        rfl = e.rfl;

    if (m & Mtd_arch::Item::RIP)
        rip = e.rip;

    if (m & Mtd_arch::Item::QUAL) {
        qual[0] = e.err;
        qual[1] = e.cr2;
        qual[2] = 0;
    }
}

bool Utcb_arch::save_exc (Mtd_arch const m, Exc_regs &e) const
{
    auto &s { e.sys };

    if (m & Mtd_arch::Item::POISON)
        return false;

    if (m & Mtd_arch::Item::GPR_0_7) {
        s.rax = rax; s.rcx = rcx; s.rdx = rdx; s.rbx = rbx;
        e.rsp = rsp; s.rbp = rbp; s.rsi = rsi; s.rdi = rdi;
    }

    if (m & Mtd_arch::Item::GPR_8_15) {
        s.r8  = r8;  s.r9  = r9;  s.r10 = r10; s.r11 = r11;
        s.r12 = r12; s.r13 = r13; s.r14 = r14; s.r15 = r15;
    }

    if (m & Mtd_arch::Item::RFLAGS)
        e.rfl = (rfl & ~(RFL_VIP | RFL_VIF | RFL_VM | RFL_RF | RFL_NT | RFL_IOPL)) | RFL_AC | RFL_IF | RFL_1;

    if (m & Mtd_arch::Item::RIP)
        e.rip = rip;

    // QUAL state is read-only

    return true;
}

void Utcb_arch::load_vmx (Mtd_arch const m, Cpu_regs const &c)
{
    auto const &s { c.exc.sys };

    c.vmcs->make_current();

    if (m & Mtd_arch::Item::GPR_0_7) {
        rax = s.rax; rcx = s.rcx; rdx = s.rdx; rbx = s.rbx;
        rbp = s.rbp; rsi = s.rsi; rdi = s.rdi;
        rsp = Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_RSP);
    }

    if (m & Mtd_arch::Item::GPR_8_15) {
        r8  = s.r8;  r9  = s.r9;  r10 = s.r10; r11 = s.r11;
        r12 = s.r12; r13 = s.r13; r14 = s.r14; r15 = s.r15;
    }

    if (m & Mtd_arch::Item::RFLAGS)
        rfl = Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_RFLAGS);

    if (m & Mtd_arch::Item::RIP) {
        rip       = Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_RIP);
        inst_len  = Vmcs::read<uint32_t>  (Vmcs::Encoding::EXI_INST_LEN);
        inst_info = Vmcs::read<uint32_t>  (Vmcs::Encoding::EXI_INST_INFO);
    }

    if (m & Mtd_arch::Item::STA) {
        intr_state = Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_INTR_STATE);
        actv_state = Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_ACTV_STATE);
    }

    if (m & Mtd_arch::Item::QUAL) {
        qual[0] = Vmcs::read<uintptr_t> (Vmcs::Encoding::EXI_QUALIFICATION);
        qual[1] = Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_LINEAR_ADDRESS);
        qual[2] = Vmcs::read<uint64_t>  (Vmcs::Encoding::GUEST_PHYSICAL_ADDRESS);
    }

    // CTRL and TPR state is write-only

    if (m & Mtd_arch::Item::INJ) {
        if (c.exc.ep() == Vmcs::Reason::VMX_FAIL_STATE || c.exc.ep() == Event::gst_arch + Event::Selector::RECALL) {
            intr_info = Vmcs::read<uint32_t> (Vmcs::Encoding::INJ_EVENT_IDENT);
            intr_errc = Vmcs::read<uint32_t> (Vmcs::Encoding::INJ_EVENT_ERROR);
        } else {
            intr_info = Vmcs::read<uint32_t> (Vmcs::Encoding::EXI_EVENT_IDENT);
            intr_errc = Vmcs::read<uint32_t> (Vmcs::Encoding::EXI_EVENT_ERROR);
            vect_info = Vmcs::read<uint32_t> (Vmcs::Encoding::ORG_EVENT_IDENT);
            vect_errc = Vmcs::read<uint32_t> (Vmcs::Encoding::ORG_EVENT_ERROR);
        }
    }

    if (m & Mtd_arch::Item::CS_SS) {
        cs.set_vmx (Vmcs::read<uint16_t> (Vmcs::Encoding::GUEST_SEL_CS), Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_BASE_CS), Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_LIMIT_CS), Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_AR_CS));
        ss.set_vmx (Vmcs::read<uint16_t> (Vmcs::Encoding::GUEST_SEL_SS), Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_BASE_SS), Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_LIMIT_SS), Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_AR_SS));
    }

    if (m & Mtd_arch::Item::DS_ES) {
        ds.set_vmx (Vmcs::read<uint16_t> (Vmcs::Encoding::GUEST_SEL_DS), Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_BASE_DS), Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_LIMIT_DS), Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_AR_DS));
        es.set_vmx (Vmcs::read<uint16_t> (Vmcs::Encoding::GUEST_SEL_ES), Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_BASE_ES), Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_LIMIT_ES), Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_AR_ES));
    }

    if (m & Mtd_arch::Item::FS_GS) {
        fs.set_vmx (Vmcs::read<uint16_t> (Vmcs::Encoding::GUEST_SEL_FS), Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_BASE_FS), Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_LIMIT_FS), Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_AR_FS));
        gs.set_vmx (Vmcs::read<uint16_t> (Vmcs::Encoding::GUEST_SEL_GS), Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_BASE_GS), Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_LIMIT_GS), Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_AR_GS));
    }

    if (m & Mtd_arch::Item::TR)
        tr.set_vmx (Vmcs::read<uint16_t> (Vmcs::Encoding::GUEST_SEL_TR), Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_BASE_TR), Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_LIMIT_TR), Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_AR_TR));

    if (m & Mtd_arch::Item::LDTR)
        ld.set_vmx (Vmcs::read<uint16_t> (Vmcs::Encoding::GUEST_SEL_LDTR), Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_BASE_LDTR), Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_LIMIT_LDTR), Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_AR_LDTR));

    if (m & Mtd_arch::Item::GDTR)
        gd.set_vmx (0, Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_BASE_GDTR), Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_LIMIT_GDTR), 0);

    if (m & Mtd_arch::Item::IDTR)
        id.set_vmx (0, Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_BASE_IDTR), Vmcs::read<uint32_t> (Vmcs::Encoding::GUEST_LIMIT_IDTR), 0);

    if (m & Mtd_arch::Item::PDPTE) {
        pdpte[0] = Vmcs::read<uint64_t> (Vmcs::Encoding::GUEST_PDPTE0);
        pdpte[1] = Vmcs::read<uint64_t> (Vmcs::Encoding::GUEST_PDPTE1);
        pdpte[2] = Vmcs::read<uint64_t> (Vmcs::Encoding::GUEST_PDPTE2);
        pdpte[3] = Vmcs::read<uint64_t> (Vmcs::Encoding::GUEST_PDPTE3);
    }

    if (m & Mtd_arch::Item::CR) {
        cr0 = c.vmx_get_gst_cr0();
        cr4 = c.vmx_get_gst_cr4();
        cr2 = c.exc.cr2;
        cr3 = Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_CR3);
    }

    if (m & Mtd_arch::Item::DR)
        dr7 = Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_DR7);

    if (m & Mtd_arch::Item::XSAVE) {
        xcr0 = c.gst_xsv.xcr;
        xss  = c.gst_xsv.xss;
    }

    if (m & Mtd_arch::Item::SYSCALL) {
        star  = c.gst_sys.star;
        lstar = c.gst_sys.lstar;
        fmask = c.gst_sys.fmask;
    }

    if (m & Mtd_arch::Item::SYSENTER) {
        sysenter_cs  = Vmcs::read<uint32_t>  (Vmcs::Encoding::GUEST_SYSENTER_CS);
        sysenter_esp = Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_SYSENTER_ESP);
        sysenter_eip = Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_SYSENTER_EIP);
    }

    if (m & Mtd_arch::Item::PAT)
        pat = Vmcs::read<uint64_t> (Vmcs::Encoding::GUEST_PAT);

    if (m & Mtd_arch::Item::EFER)
        efer = Vmcs::read<uint64_t> (Vmcs::Encoding::GUEST_EFER);

    if (m & Mtd_arch::Item::KERNEL_GS_BASE)
        kernel_gs_base = c.gst_sys.kernel_gs_base;

    if (m & Mtd_arch::Item::TSC)
        tsc_aux = c.gst_tsc.tsc_aux;
}

bool Utcb_arch::save_vmx (Mtd_arch const m, Cpu_regs &c, Space_obj const *obj) const
{
    auto &s { c.exc.sys };

    c.vmcs->make_current();

    if (m & Mtd_arch::Item::POISON)
        return false;

    if (m & Mtd_arch::Item::GPR_0_7) {
        s.rax = rax; s.rcx = rcx; s.rdx = rdx; s.rbx = rbx;
        s.rbp = rbp; s.rsi = rsi; s.rdi = rdi;
        Vmcs::write (Vmcs::Encoding::GUEST_RSP, rsp);
    }

    if (m & Mtd_arch::Item::GPR_8_15) {
        s.r8  = r8;  s.r9  = r9;  s.r10 = r10; s.r11 = r11;
        s.r12 = r12; s.r13 = r13; s.r14 = r14; s.r15 = r15;
    }

    if (m & Mtd_arch::Item::RFLAGS)
        Vmcs::write (Vmcs::Encoding::GUEST_RFLAGS, rfl);

    if (m & Mtd_arch::Item::RIP) {
        Vmcs::write (Vmcs::Encoding::GUEST_RIP, rip);
        Vmcs::write (Vmcs::Encoding::ENT_INST_LEN, inst_len);
    }

    if (m & Mtd_arch::Item::STA) {
        Vmcs::write (Vmcs::Encoding::GUEST_INTR_STATE, intr_state);
        Vmcs::write (Vmcs::Encoding::GUEST_ACTV_STATE, actv_state);
    }

    // QUAL state is read-only

    if (m & Mtd_arch::Item::CTRL) {
        c.exc.intcpt_cr0 = intcpt_cr0;
        c.exc.intcpt_cr4 = intcpt_cr4;
        c.exc.intcpt_exc = intcpt_exc;
        c.vmx_set_msk_cr0();
        c.vmx_set_msk_cr4();
        c.vmx_set_bmp_exc();
        c.vmx_set_cpu_pri (ctrl_pri);
        c.vmx_set_cpu_sec (ctrl_sec);
        c.vmx_set_cpu_ter (ctrl_ter);
        Vmcs::write (Vmcs::Encoding::PF_ERROR_MASK,  pfe_mask);
        Vmcs::write (Vmcs::Encoding::PF_ERROR_MATCH, pfe_match);
    }

    if (m & Mtd_arch::Item::TPR)
        Vmcs::write (Vmcs::Encoding::TPR_THRESHOLD, tpr_threshold);

    if (m & Mtd_arch::Item::INJ) {

        auto val { Vmcs::read<uint32_t> (Vmcs::Encoding::CPU_CONTROLS_PRI) };

        if (intr_info & 0x1000)
            val |=  Vmcs::CPU_INTR_WINDOW;
        else
            val &= ~Vmcs::CPU_INTR_WINDOW;

        if (intr_info & 0x2000)
            val |=  Vmcs::CPU_NMI_WINDOW;
        else
            val &= ~Vmcs::CPU_NMI_WINDOW;

        c.vmx_set_cpu_pri (val);

        Vmcs::write (Vmcs::Encoding::INJ_EVENT_IDENT, intr_info & ~0x3000);
        Vmcs::write (Vmcs::Encoding::INJ_EVENT_ERROR, intr_errc);
    }

    if (m & Mtd_arch::Item::CS_SS) {
        Vmcs::write (Vmcs::Encoding::GUEST_SEL_CS,   cs.sel);
        Vmcs::write (Vmcs::Encoding::GUEST_BASE_CS,  cs.base);
        Vmcs::write (Vmcs::Encoding::GUEST_LIMIT_CS, cs.limit);
        Vmcs::write (Vmcs::Encoding::GUEST_AR_CS,   (cs.ar << 4 & 0x1f000) | (cs.ar & 0xff));
        Vmcs::write (Vmcs::Encoding::GUEST_SEL_SS,   ss.sel);
        Vmcs::write (Vmcs::Encoding::GUEST_BASE_SS,  ss.base);
        Vmcs::write (Vmcs::Encoding::GUEST_LIMIT_SS, ss.limit);
        Vmcs::write (Vmcs::Encoding::GUEST_AR_SS,   (ss.ar << 4 & 0x1f000) | (ss.ar & 0xff));
    }

    if (m & Mtd_arch::Item::DS_ES) {
        Vmcs::write (Vmcs::Encoding::GUEST_SEL_DS,   ds.sel);
        Vmcs::write (Vmcs::Encoding::GUEST_BASE_DS,  ds.base);
        Vmcs::write (Vmcs::Encoding::GUEST_LIMIT_DS, ds.limit);
        Vmcs::write (Vmcs::Encoding::GUEST_AR_DS,   (ds.ar << 4 & 0x1f000) | (ds.ar & 0xff));
        Vmcs::write (Vmcs::Encoding::GUEST_SEL_ES,   es.sel);
        Vmcs::write (Vmcs::Encoding::GUEST_BASE_ES,  es.base);
        Vmcs::write (Vmcs::Encoding::GUEST_LIMIT_ES, es.limit);
        Vmcs::write (Vmcs::Encoding::GUEST_AR_ES,   (es.ar << 4 & 0x1f000) | (es.ar & 0xff));
    }

    if (m & Mtd_arch::Item::FS_GS) {
        Vmcs::write (Vmcs::Encoding::GUEST_SEL_FS,   fs.sel);
        Vmcs::write (Vmcs::Encoding::GUEST_BASE_FS,  fs.base);
        Vmcs::write (Vmcs::Encoding::GUEST_LIMIT_FS, fs.limit);
        Vmcs::write (Vmcs::Encoding::GUEST_AR_FS,   (fs.ar << 4 & 0x1f000) | (fs.ar & 0xff));
        Vmcs::write (Vmcs::Encoding::GUEST_SEL_GS,   gs.sel);
        Vmcs::write (Vmcs::Encoding::GUEST_BASE_GS,  gs.base);
        Vmcs::write (Vmcs::Encoding::GUEST_LIMIT_GS, gs.limit);
        Vmcs::write (Vmcs::Encoding::GUEST_AR_GS,   (gs.ar << 4 & 0x1f000) | (gs.ar & 0xff));
    }

    if (m & Mtd_arch::Item::TR) {
        Vmcs::write (Vmcs::Encoding::GUEST_SEL_TR,   tr.sel);
        Vmcs::write (Vmcs::Encoding::GUEST_BASE_TR,  tr.base);
        Vmcs::write (Vmcs::Encoding::GUEST_LIMIT_TR, tr.limit);
        Vmcs::write (Vmcs::Encoding::GUEST_AR_TR,   (tr.ar << 4 & 0x1f000) | (tr.ar & 0xff));
    }

    if (m & Mtd_arch::Item::LDTR) {
        Vmcs::write (Vmcs::Encoding::GUEST_SEL_LDTR,   ld.sel);
        Vmcs::write (Vmcs::Encoding::GUEST_BASE_LDTR,  ld.base);
        Vmcs::write (Vmcs::Encoding::GUEST_LIMIT_LDTR, ld.limit);
        Vmcs::write (Vmcs::Encoding::GUEST_AR_LDTR,   (ld.ar << 4 & 0x1f000) | (ld.ar & 0xff));
    }

    if (m & Mtd_arch::Item::GDTR) {
        Vmcs::write (Vmcs::Encoding::GUEST_BASE_GDTR,  gd.base);
        Vmcs::write (Vmcs::Encoding::GUEST_LIMIT_GDTR, gd.limit);
    }

    if (m & Mtd_arch::Item::IDTR) {
        Vmcs::write (Vmcs::Encoding::GUEST_BASE_IDTR,  id.base);
        Vmcs::write (Vmcs::Encoding::GUEST_LIMIT_IDTR, id.limit);
    }

    if (m & Mtd_arch::Item::PDPTE) {
        Vmcs::write (Vmcs::Encoding::GUEST_PDPTE0, pdpte[0]);
        Vmcs::write (Vmcs::Encoding::GUEST_PDPTE1, pdpte[1]);
        Vmcs::write (Vmcs::Encoding::GUEST_PDPTE2, pdpte[2]);
        Vmcs::write (Vmcs::Encoding::GUEST_PDPTE3, pdpte[3]);
    }

    if (m & Mtd_arch::Item::CR) {
        c.vmx_set_gst_cr0 (cr0);
        c.vmx_set_gst_cr4 (cr4);
        c.exc.cr2 = cr2;
        Vmcs::write (Vmcs::Encoding::GUEST_CR3, cr3);
    }

    if (m & Mtd_arch::Item::DR)
        Vmcs::write (Vmcs::Encoding::GUEST_DR7, dr7);

    if (m & Mtd_arch::Item::XSAVE) {
        c.gst_xsv.xcr = Fpu::State_xsv::constrain_xcr (xcr0);
        c.gst_xsv.xss = Fpu::State_xsv::constrain_xss (xss);
    }

    if (m & Mtd_arch::Item::SYSCALL) {
        c.gst_sys.star  = Cpu::State_sys::constrain_star  (star);
        c.gst_sys.lstar = Cpu::State_sys::constrain_canon (lstar);
        c.gst_sys.fmask = Cpu::State_sys::constrain_fmask (fmask);
    }

    if (m & Mtd_arch::Item::SYSENTER) {
        Vmcs::write (Vmcs::Encoding::GUEST_SYSENTER_CS,  sysenter_cs);
        Vmcs::write (Vmcs::Encoding::GUEST_SYSENTER_ESP, sysenter_esp);
        Vmcs::write (Vmcs::Encoding::GUEST_SYSENTER_EIP, sysenter_eip);
    }

    if (m & Mtd_arch::Item::PAT)
        Vmcs::write (Vmcs::Encoding::GUEST_PAT, pat);

    if (m & Mtd_arch::Item::EFER) {

        Vmcs::write (Vmcs::Encoding::GUEST_EFER, efer);

        auto ent { Vmcs::read<uint32_t> (Vmcs::Encoding::ENT_CONTROLS) };

        if (efer & EFER_LMA)
            ent |=  Vmcs::ENT_GUEST_64;
        else
            ent &= ~Vmcs::ENT_GUEST_64;

        Vmcs::write (Vmcs::Encoding::ENT_CONTROLS, ent);
    }

    if (m & Mtd_arch::Item::KERNEL_GS_BASE)
        c.gst_sys.kernel_gs_base = Cpu::State_sys::constrain_canon (kernel_gs_base);

    if (m & Mtd_arch::Item::TSC)
        c.gst_tsc.tsc_aux = Cpu::State_tsc::constrain_tsc_aux (tsc_aux);

    if (m & Mtd_arch::Item::TLB) {

        auto vpid = Vmcs::vpid();

        if (vpid)
            Vpid::invalidate (Vmcs::has_invvpid_sgl() ? Invvpid::Type::SGL : Invvpid::Type::ALL, vpid);
    }

    if (m & Mtd_arch::Item::SPACES) {

        if (EXPECT_FALSE (!assign_spaces (c, obj)))
            return false;

        Vmcs::write (Vmcs::Encoding::EPTP,        c.gst->get_phys() | (Ept::lev() - 1) << 3 | CA_TYPE_MEM_WB);
        Vmcs::write (Vmcs::Encoding::BITMAP_IO_A, c.pio->get_phys());
        Vmcs::write (Vmcs::Encoding::BITMAP_IO_B, c.pio->get_phys() + PAGE_SIZE (0));
        Vmcs::write (Vmcs::Encoding::BITMAP_MSR,  c.msr->get_phys());
    }

    if (m & Mtd_arch::Item::APIC) {

        uint64_t addr;

        if (EXPECT_FALSE (!assign_aapage (c, addr)))
            return false;

        Vmcs::write (Vmcs::Encoding::APIC_ACCS_ADDR, addr);
    }

    return true;
}

void Utcb_arch::load_svm (Mtd_arch const m, Cpu_regs const &c)
{
    auto const &s { c.exc.sys };
    auto const  v { c.vmcb };

    if (m & Mtd_arch::Item::GPR_0_7) {
        rax = v->rax; rcx = s.rcx; rdx = s.rdx; rbx = s.rbx;
        rsp = v->rsp; rbp = s.rbp; rsi = s.rsi; rdi = s.rdi;
    }

    if (m & Mtd_arch::Item::GPR_8_15) {
        r8  = s.r8;  r9  = s.r9;  r10 = s.r10; r11 = s.r11;
        r12 = s.r12; r13 = s.r13; r14 = s.r14; r15 = s.r15;
    }

    if (m & Mtd_arch::Item::RFLAGS)
        rfl = v->rflags;

    if (m & Mtd_arch::Item::RIP)
        rip = v->rip;

    if (m & Mtd_arch::Item::STA) {
        intr_state = static_cast<uint32_t>(v->int_shadow);
        actv_state = 0;
    }

    if (m & Mtd_arch::Item::QUAL) {
        qual[0] = v->exitinfo1;
        qual[1] = v->exitinfo2;
        qual[2] = 0;
    }

    // CTRL and TPR state is write-only

    if (m & Mtd_arch::Item::INJ) {
        if (c.exc.ep() == NUM_VMI - 3 || c.exc.ep() == Event::gst_arch + Event::Selector::RECALL) {
            intr_info = static_cast<uint32_t>(v->inj_control);
            intr_errc = static_cast<uint32_t>(v->inj_control >> 32);
        } else {
            intr_info = static_cast<uint32_t>(v->exitcode);
            intr_errc = static_cast<uint32_t>(v->exitinfo1);
            vect_info = static_cast<uint32_t>(v->exitintinfo);
            vect_errc = static_cast<uint32_t>(v->exitintinfo >> 32);
        }
    }

    if (m & Mtd_arch::Item::CS_SS) {
        cs = v->cs;
        ss = v->ss;
    }

    if (m & Mtd_arch::Item::DS_ES) {
        ds = v->ds;
        es = v->es;
    }

    if (m & Mtd_arch::Item::FS_GS) {
        fs = v->fs;
        gs = v->gs;
    }

    if (m & Mtd_arch::Item::TR)
        tr = v->tr;

    if (m & Mtd_arch::Item::LDTR)
        ld = v->ldtr;

    if (m & Mtd_arch::Item::GDTR)
        gd = v->gdtr;

    if (m & Mtd_arch::Item::IDTR)
        id = v->idtr;

    // PDPTE registers are not used

    if (m & Mtd_arch::Item::CR) {
        cr0 = v->cr0;
        cr2 = v->cr2;
        cr3 = v->cr3;
        cr4 = v->cr4;
    }

    if (m & Mtd_arch::Item::DR)
        dr7 = v->dr7;

    if (m & Mtd_arch::Item::XSAVE) {
        xcr0 = c.gst_xsv.xcr;
        xss  = c.gst_xsv.xss;
    }

    if (m & Mtd_arch::Item::SYSCALL) {
        star  = v->star;
        lstar = v->lstar;
        fmask = v->sfmask;
    }

    if (m & Mtd_arch::Item::SYSENTER) {
        sysenter_cs  = v->sysenter_cs;
        sysenter_esp = v->sysenter_esp;
        sysenter_eip = v->sysenter_eip;
    }

    if (m & Mtd_arch::Item::PAT)
        pat = v->g_pat;

    if (m & Mtd_arch::Item::EFER)
        efer = v->efer;

    if (m & Mtd_arch::Item::KERNEL_GS_BASE)
        kernel_gs_base = v->kernel_gs_base;

    if (m & Mtd_arch::Item::TSC)
        tsc_aux = c.gst_tsc.tsc_aux;
}

bool Utcb_arch::save_svm (Mtd_arch const m, Cpu_regs &c, Space_obj const *obj) const
{
    auto &s { c.exc.sys };
    auto  v { c.vmcb };

    if (m & Mtd_arch::Item::POISON)
        return false;

    if (m & Mtd_arch::Item::GPR_0_7) {
        v->rax = rax; s.rcx = rcx; s.rdx = rdx; s.rbx = rbx;
        v->rsp = rsp; s.rbp = rbp; s.rsi = rsi; s.rdi = rdi;
    }

    if (m & Mtd_arch::Item::GPR_8_15) {
        s.r8  = r8;  s.r9  = r9;  s.r10 = r10; s.r11 = r11;
        s.r12 = r12; s.r13 = r13; s.r14 = r14; s.r15 = r15;
    }

    if (m & Mtd_arch::Item::RFLAGS)
        v->rflags = rfl;

    if (m & Mtd_arch::Item::RIP)
        v->rip = rip;

    if (m & Mtd_arch::Item::STA)
        v->int_shadow = intr_state;

    // QUAL state is read-only

    if (m & Mtd_arch::Item::CTRL) {
        c.exc.intcpt_cr0 = intcpt_cr0;
        c.exc.intcpt_cr4 = intcpt_cr4;
        c.exc.intcpt_exc = intcpt_exc;
        c.svm_set_bmp_exc();
        c.svm_set_cpu_pri (ctrl_pri);
        c.svm_set_cpu_sec (ctrl_sec);
    }

    // TPR threshold is not used

    if (m & Mtd_arch::Item::INJ) {

        if (intr_info & 0x1000) {
            v->int_control      |=  (1ul << 8 | 1ul << 20);
            v->intercept_cpu[0] |=  Vmcb::CPU_VINTR;
        } else {
            v->int_control      &= ~(1ul << 8 | 1ul << 20);
            v->intercept_cpu[0] &= ~Vmcb::CPU_VINTR;
        }

        v->inj_control = static_cast<uint64_t>(intr_errc) << 32 | (intr_info & ~0x3000);
    }

    if (m & Mtd_arch::Item::CS_SS) {
        v->cs = cs;
        v->ss = ss;
    }

    if (m & Mtd_arch::Item::DS_ES) {
        v->ds = ds;
        v->es = es;
    }

    if (m & Mtd_arch::Item::FS_GS) {
        v->fs = fs;
        v->gs = gs;
    }

    if (m & Mtd_arch::Item::TR)
        v->tr = tr;

    if (m & Mtd_arch::Item::LDTR)
        v->ldtr = ld;

    if (m & Mtd_arch::Item::GDTR)
        v->gdtr = gd;

    if (m & Mtd_arch::Item::IDTR)
        v->idtr = id;

    // PDPTE registers are not used

    if (m & Mtd_arch::Item::CR) {
        v->cr0 = cr0;
        v->cr2 = cr2;
        v->cr3 = cr3;
        v->cr4 = cr4;
    }

    if (m & Mtd_arch::Item::DR)
        v->dr7 = dr7;

    if (m & Mtd_arch::Item::XSAVE) {
        c.gst_xsv.xcr = Fpu::State_xsv::constrain_xcr (xcr0);
        c.gst_xsv.xss = Fpu::State_xsv::constrain_xss (xss);
    }

    if (m & Mtd_arch::Item::SYSCALL) {
        v->star   = Cpu::State_sys::constrain_star  (star);
        v->lstar  = Cpu::State_sys::constrain_canon (lstar);
        v->sfmask = Cpu::State_sys::constrain_fmask (fmask);
    }

    if (m & Mtd_arch::Item::SYSENTER) {
        v->sysenter_cs  = sysenter_cs;
        v->sysenter_esp = sysenter_esp;
        v->sysenter_eip = sysenter_eip;
    }

    if (m & Mtd_arch::Item::PAT)
        v->g_pat = pat;

    if (m & Mtd_arch::Item::EFER)
        v->efer = efer;

    if (m & Mtd_arch::Item::KERNEL_GS_BASE)
        v->kernel_gs_base = Cpu::State_sys::constrain_canon (kernel_gs_base);

    if (m & Mtd_arch::Item::TSC)
        c.gst_tsc.tsc_aux = Cpu::State_tsc::constrain_tsc_aux (tsc_aux);

    if (m & Mtd_arch::Item::TLB)
        if (v->asid)
            v->tlb_control = 3;

    if (m & Mtd_arch::Item::SPACES) {

        if (EXPECT_FALSE (!assign_spaces (c, obj)))
            return false;

        v->npt_cr3  = c.gst->get_phys();
        v->base_io  = c.pio->get_phys();
        v->base_msr = c.msr->get_phys();
    }

    return true;
}

bool Utcb_arch::assign_aapage (Cpu_regs &c, uint64_t &addr) const
{
    Space_gst const *const gst { c.gst };

    unsigned o; Memattr ma;

    if (EXPECT_FALSE (!gst || !gst->lookup (apic_base & ~OFFS_MASK (0), addr, o, ma) || o))
        return false;

    addr |= ma.key_encode();

    return true;
}

bool Utcb_arch::assign_spaces (Cpu_regs &c, Space_obj const *obj) const
{
    auto const cap_gst { obj->lookup (sel.gst) };
    auto const cap_pio { obj->lookup (sel.pio) };
    auto const cap_msr { obj->lookup (sel.msr) };

    // Space capabilities must have ASSIGN permission
    if (EXPECT_FALSE (!cap_gst.validate (Capability::Perm_sp::ASSIGN, Kobject::Subtype::GST) ||
                      !cap_pio.validate (Capability::Perm_sp::ASSIGN, Kobject::Subtype::PIO) ||
                      !cap_msr.validate (Capability::Perm_sp::ASSIGN, Kobject::Subtype::MSR)))
        return false;

    auto const gst { static_cast<Space_gst *>(cap_gst.obj()) };
    auto const pio { static_cast<Space_pio *>(cap_pio.obj()) };
    auto const msr { static_cast<Space_msr *>(cap_msr.obj()) };
    auto const own { c.obj->get_pd() };

    // Spaces must belong to the EC's PD
    if (EXPECT_FALSE (gst->get_pd() != own || pio->get_pd() != own || msr->get_pd() != own))
        return false;

    // Acquire references
    Refptr<Space_gst> ref_gst { gst };
    Refptr<Space_pio> ref_pio { pio };
    Refptr<Space_msr> ref_msr { msr };

    // Failed to acquire references
    if (EXPECT_FALSE (!ref_gst || !ref_pio || !ref_msr))
        return false;

    c.gst = std::move (ref_gst);
    c.pio = std::move (ref_pio);
    c.msr = std::move (ref_msr);

    // References must have been consumed
    assert (!ref_gst && !ref_pio && !ref_msr);

    c.hazard.clr (Hazard::ILLEGAL);

    return true;
}
