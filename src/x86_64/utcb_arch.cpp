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
#include "lowlevel.hpp"
#include "mtd_arch.hpp"
#include "regs.hpp"
#include "svm.hpp"
#include "vmx.hpp"

void Utcb_arch::load_exc (Mtd_arch const m, Cpu_regs const *r)
{
    if (m & Mtd_arch::Item::GPR_0_7) {
        rax = r->rax; rcx = r->rcx; rdx = r->rdx; rbx = r->rbx;
        rsp = r->rsp; rbp = r->rbp; rsi = r->rsi; rdi = r->rdi;
    }

    if (m & Mtd_arch::Item::GPR_8_15) {
        r8  = r->r8;  r9  = r->r9;  r10 = r->r10; r11 = r->r11;
        r12 = r->r12; r13 = r->r13; r14 = r->r14; r15 = r->r15;
    }

    if (m & Mtd_arch::Item::RFLAGS)
        rfl = r->rfl;

    if (m & Mtd_arch::Item::RIP)
        rip = r->rip;

    if (m & Mtd_arch::Item::QUAL) {
        qual[0] = r->err;
        qual[1] = r->cr2;
    }
}

bool Utcb_arch::save_exc (Mtd_arch const m, Cpu_regs *r) const
{
    if (m & Mtd_arch::Item::POISON)
        return false;

    if (m & Mtd_arch::Item::GPR_0_7) {
        r->rax = rax; r->rcx = rcx; r->rdx = rdx; r->rbx = rbx;
        r->rsp = rsp; r->rbp = rbp; r->rsi = rsi; r->rdi = rdi;
    }

    if (m & Mtd_arch::Item::GPR_8_15) {
        r->r8  = r8;  r->r9  = r9;  r->r10 = r10; r->r11 = r11;
        r->r12 = r12; r->r13 = r13; r->r14 = r14; r->r15 = r15;
    }

    if (m & Mtd_arch::Item::RFLAGS)
        r->rfl = (rfl & ~(RFL_VIP | RFL_VIF | RFL_VM | RFL_RF | RFL_NT | RFL_IOPL)) | RFL_AC | RFL_IF;

    if (m & Mtd_arch::Item::RIP)
        r->rip = rip;

    // QUAL state is read-only

    return true;
}

void Utcb_arch::load_vmx (Mtd_arch const m, Cpu_regs const *r)
{
    r->vmcs->make_current();

    if (m & Mtd_arch::Item::GPR_0_7) {
        rax = r->rax; rcx = r->rcx; rdx = r->rdx; rbx = r->rbx;
        rbp = r->rbp; rsi = r->rsi; rdi = r->rdi;
        rsp = Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_RSP);
    }

    if (m & Mtd_arch::Item::GPR_8_15) {
        r8  = r->r8;  r9  = r->r9;  r10 = r->r10; r11 = r->r11;
        r12 = r->r12; r13 = r->r13; r14 = r->r14; r15 = r->r15;
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
        if (r->dst_portal == 33 || r->dst_portal == NUM_VMI - 1) {
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
        cr0 = r->get_cr0<Vmcs>();
        cr4 = r->get_cr4<Vmcs>();
        cr2 = r->cr2;
        cr3 = Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_CR3);
    }

    if (m & Mtd_arch::Item::DR)
        dr7 = Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_DR7);

    if (m & Mtd_arch::Item::SYSENTER) {
        sysenter_cs  = Vmcs::read<uint32_t>  (Vmcs::Encoding::GUEST_SYSENTER_CS);
        sysenter_esp = Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_SYSENTER_ESP);
        sysenter_eip = Vmcs::read<uintptr_t> (Vmcs::Encoding::GUEST_SYSENTER_EIP);
    }

    if (m & Mtd_arch::Item::PAT)
        pat = Vmcs::read<uint64_t> (Vmcs::Encoding::GUEST_PAT);

    if (m & Mtd_arch::Item::EFER)
        efer = Vmcs::read<uint64_t> (Vmcs::Encoding::GUEST_EFER);
}

bool Utcb_arch::save_vmx (Mtd_arch const m, Cpu_regs *r) const
{
    r->vmcs->make_current();

    if (m & Mtd_arch::Item::POISON)
        return false;

    if (m & Mtd_arch::Item::GPR_0_7) {
        r->rax = rax; r->rcx = rcx; r->rdx = rdx; r->rbx = rbx;
        r->rbp = rbp; r->rsi = rsi; r->rdi = rdi;
        Vmcs::write (Vmcs::Encoding::GUEST_RSP, rsp);
    }

    if (m & Mtd_arch::Item::GPR_8_15) {
        r->r8  = r8;  r->r9  = r9;  r->r10 = r10; r->r11 = r11;
        r->r12 = r12; r->r13 = r13; r->r14 = r14; r->r15 = r15;
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
        r->set_cpu_ctrl0<Vmcs> (ctrl_pri);
        r->set_cpu_ctrl1<Vmcs> (ctrl_sec);
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

        r->set_cpu_ctrl0<Vmcs> (val);

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
        r->set_cr0<Vmcs> (cr0);
        r->set_cr4<Vmcs> (cr4);
        r->cr2 = cr2;
        Vmcs::write (Vmcs::Encoding::GUEST_CR3, cr3);
    }

    if (m & Mtd_arch::Item::DR)
        Vmcs::write (Vmcs::Encoding::GUEST_DR7, dr7);

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

    return true;
}

void Utcb_arch::load_svm (Mtd_arch const m, Cpu_regs const *r)
{
    Vmcb const *const vmcb = r->vmcb;

    if (m & Mtd_arch::Item::GPR_0_7) {
        rax = vmcb->rax; rcx = r->rcx; rdx = r->rdx; rbx = r->rbx;
        rsp = vmcb->rsp; rbp = r->rbp; rsi = r->rsi; rdi = r->rdi;
    }

    if (m & Mtd_arch::Item::GPR_8_15) {
        r8  = r->r8;  r9  = r->r9;  r10 = r->r10; r11 = r->r11;
        r12 = r->r12; r13 = r->r13; r14 = r->r14; r15 = r->r15;
    }

    if (m & Mtd_arch::Item::RFLAGS)
        rfl = vmcb->rflags;

    if (m & Mtd_arch::Item::RIP)
        rip = vmcb->rip;

    if (m & Mtd_arch::Item::STA) {
        intr_state = static_cast<uint32_t>(vmcb->int_shadow);
        actv_state = 0;
    }

    if (m & Mtd_arch::Item::QUAL) {
        qual[0] = vmcb->exitinfo1;
        qual[1] = vmcb->exitinfo2;
    }

    // CTRL and TPR state is write-only

    if (m & Mtd_arch::Item::INJ) {
        if (r->dst_portal == NUM_VMI - 3 || r->dst_portal == NUM_VMI - 1) {
            intr_info = static_cast<uint32_t>(vmcb->inj_control);
            intr_errc = static_cast<uint32_t>(vmcb->inj_control >> 32);
        } else {
            intr_info = static_cast<uint32_t>(vmcb->exitcode);
            intr_errc = static_cast<uint32_t>(vmcb->exitinfo1);
            vect_info = static_cast<uint32_t>(vmcb->exitintinfo);
            vect_errc = static_cast<uint32_t>(vmcb->exitintinfo >> 32);
        }
    }

    if (m & Mtd_arch::Item::CS_SS) {
        cs = vmcb->cs;
        ss = vmcb->ss;
    }

    if (m & Mtd_arch::Item::DS_ES) {
        ds = vmcb->ds;
        es = vmcb->es;
    }

    if (m & Mtd_arch::Item::FS_GS) {
        fs = vmcb->fs;
        gs = vmcb->gs;
    }

    if (m & Mtd_arch::Item::TR)
        tr = vmcb->tr;

    if (m & Mtd_arch::Item::LDTR)
        ld = vmcb->ldtr;

    if (m & Mtd_arch::Item::GDTR)
        gd = vmcb->gdtr;

    if (m & Mtd_arch::Item::IDTR)
        id = vmcb->idtr;

    // PDPTE registers are not used

    if (m & Mtd_arch::Item::CR) {
        cr0 = r->get_cr0<Vmcb>();
        cr4 = r->get_cr4<Vmcb>();
        cr2 = vmcb->cr2;
        cr3 = vmcb->cr3;
    }

    if (m & Mtd_arch::Item::DR)
        dr7 = vmcb->dr7;

    if (m & Mtd_arch::Item::SYSENTER) {
        sysenter_cs  = vmcb->sysenter_cs;
        sysenter_esp = vmcb->sysenter_esp;
        sysenter_eip = vmcb->sysenter_eip;
    }

    if (m & Mtd_arch::Item::PAT)
        pat = vmcb->g_pat;

    if (m & Mtd_arch::Item::EFER)
        efer = vmcb->efer;
}

bool Utcb_arch::save_svm (Mtd_arch const m, Cpu_regs *r) const
{
    Vmcb * const vmcb = r->vmcb;

    if (m & Mtd_arch::Item::POISON)
        return false;

    if (m & Mtd_arch::Item::GPR_0_7) {
        vmcb->rax = rax; r->rcx = rcx; r->rdx = rdx; r->rbx = rbx;
        vmcb->rsp = rsp; r->rbp = rbp; r->rsi = rsi; r->rdi = rdi;
    }

    if (m & Mtd_arch::Item::GPR_8_15) {
        r->r8  = r8;  r->r9  = r9;  r->r10 = r10; r->r11 = r11;
        r->r12 = r12; r->r13 = r13; r->r14 = r14; r->r15 = r15;
    }

    if (m & Mtd_arch::Item::RFLAGS)
        vmcb->rflags = rfl;

    if (m & Mtd_arch::Item::RIP)
        vmcb->rip = rip;

    if (m & Mtd_arch::Item::STA)
        vmcb->int_shadow = intr_state;

    // QUAL state is read-only

    if (m & Mtd_arch::Item::CTRL) {
        r->set_cpu_ctrl0<Vmcb> (ctrl_pri);
        r->set_cpu_ctrl1<Vmcb> (ctrl_sec);
    }

    // TPR threshold is not used

    if (m & Mtd_arch::Item::INJ) {

        if (intr_info & 0x1000) {
            vmcb->int_control      |=  (1ul << 8 | 1ul << 20);
            vmcb->intercept_cpu[0] |=  Vmcb::CPU_VINTR;
        } else {
            vmcb->int_control      &= ~(1ul << 8 | 1ul << 20);
            vmcb->intercept_cpu[0] &= ~Vmcb::CPU_VINTR;
        }

        vmcb->inj_control = static_cast<uint64_t>(intr_errc) << 32 | (intr_info & ~0x3000);
    }

    if (m & Mtd_arch::Item::CS_SS) {
        vmcb->cs = cs;
        vmcb->ss = ss;
    }

    if (m & Mtd_arch::Item::DS_ES) {
        vmcb->ds = ds;
        vmcb->es = es;
    }

    if (m & Mtd_arch::Item::FS_GS) {
        vmcb->fs = fs;
        vmcb->gs = gs;
    }

    if (m & Mtd_arch::Item::TR)
        vmcb->tr = tr;

    if (m & Mtd_arch::Item::LDTR)
        vmcb->ldtr = ld;

    if (m & Mtd_arch::Item::GDTR)
        vmcb->gdtr = gd;

    if (m & Mtd_arch::Item::IDTR)
        vmcb->idtr = id;

    // PDPTE registers are not used

    if (m & Mtd_arch::Item::CR) {
        r->set_cr0<Vmcb> (cr0);
        r->set_cr4<Vmcb> (cr4);
        vmcb->cr2 = cr2;
        vmcb->cr3 = cr3;
    }

    if (m & Mtd_arch::Item::DR)
        vmcb->dr7 = dr7;

    if (m & Mtd_arch::Item::SYSENTER) {
        vmcb->sysenter_cs  = sysenter_cs;
        vmcb->sysenter_esp = sysenter_esp;
        vmcb->sysenter_eip = sysenter_eip;
    }

    if (m & Mtd_arch::Item::PAT)
        vmcb->g_pat = pat;

    if (m & Mtd_arch::Item::EFER)
        vmcb->efer = efer;

    return true;
}
