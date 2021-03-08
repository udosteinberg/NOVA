/*
 * User Thread Control Block (UTCB)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

#include "barrier.hpp"
#include "config.hpp"
#include "cpu.hpp"
#include "lowlevel.hpp"
#include "mtd_arch.hpp"
#include "regs.hpp"
#include "svm.hpp"
#include "vmx.hpp"

bool Utcb::load_exc (Cpu_regs *regs)
{
    mword m = regs->mtd;

    if (m & Mtd_arch::GPR_0_7) {
        rax = regs->rax;
        rcx = regs->rcx;
        rdx = regs->rdx;
        rbx = regs->rbx;
        rsp = regs->rsp;
        rbp = regs->rbp;
        rsi = regs->rsi;
        rdi = regs->rdi;
    }

    if (m & Mtd_arch::GPR_8_15) {
        r8  = regs->r8;
        r9  = regs->r9;
        r10 = regs->r10;
        r11 = regs->r11;
        r12 = regs->r12;
        r13 = regs->r13;
        r14 = regs->r14;
        r15 = regs->r15;
    }

    if (m & Mtd_arch::RIP)
        rip = regs->rip;

    if (m & Mtd_arch::RFLAGS)
        rflags = regs->rfl;

    if (m & Mtd_arch::QUAL) {
        qual[0] = regs->err;
        qual[1] = regs->cr2;
    }

    barrier();
    mtd = m;
    items = sizeof (Utcb_data) / sizeof (mword);

    return m & Mtd_arch::FPU;
}

bool Utcb::save_exc (Cpu_regs *regs)
{
    if (mtd & Mtd_arch::GPR_0_7) {
        regs->rax = rax;
        regs->rcx = rcx;
        regs->rdx = rdx;
        regs->rbx = rbx;
        regs->rsp = rsp;
        regs->rbp = rbp;
        regs->rsi = rsi;
        regs->rdi = rdi;
    }

    if (mtd & Mtd_arch::GPR_8_15) {
        regs->r8  = r8;
        regs->r9  = r9;
        regs->r10 = r10;
        regs->r11 = r11;
        regs->r12 = r12;
        regs->r13 = r13;
        regs->r14 = r14;
        regs->r15 = r15;
    }

    if (mtd & Mtd_arch::RIP)
        regs->rip = rip;

    if (mtd & Mtd_arch::RFLAGS)
        regs->rfl = (rflags & ~(RFL_VIP | RFL_VIF | RFL_VM | RFL_RF | RFL_IOPL)) | RFL_IF;

    return mtd & Mtd_arch::FPU;
}

bool Utcb::load_vmx (Cpu_regs *regs)
{
    regs->vmcs->make_current();

    mword m = regs->mtd;

    if (m & Mtd_arch::GPR_0_7) {
        rax = regs->rax;
        rcx = regs->rcx;
        rdx = regs->rdx;
        rbx = regs->rbx;
        rsp = Vmcs::read<mword> (Vmcs::GUEST_RSP);
        rbp = regs->rbp;
        rsi = regs->rsi;
        rdi = regs->rdi;
    }

    if (m & Mtd_arch::GPR_8_15) {
        r8  = regs->r8;
        r9  = regs->r9;
        r10 = regs->r10;
        r11 = regs->r11;
        r12 = regs->r12;
        r13 = regs->r13;
        r14 = regs->r14;
        r15 = regs->r15;
    }

    if (m & Mtd_arch::RIP) {
        rip      = Vmcs::read<mword>  (Vmcs::GUEST_RIP);
        inst_len = Vmcs::read<uint32> (Vmcs::EXI_INST_LEN);
    }

    if (m & Mtd_arch::RFLAGS)
        rflags = Vmcs::read<mword> (Vmcs::GUEST_RFLAGS);

    if (m & Mtd_arch::DS_ES) {
        ds.set_vmx (Vmcs::read<uint16> (Vmcs::GUEST_SEL_DS), Vmcs::read<mword> (Vmcs::GUEST_BASE_DS), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_DS), Vmcs::read<uint32> (Vmcs::GUEST_AR_DS));
        es.set_vmx (Vmcs::read<uint16> (Vmcs::GUEST_SEL_ES), Vmcs::read<mword> (Vmcs::GUEST_BASE_ES), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_ES), Vmcs::read<uint32> (Vmcs::GUEST_AR_ES));
    }

    if (m & Mtd_arch::FS_GS) {
        fs.set_vmx (Vmcs::read<uint16> (Vmcs::GUEST_SEL_FS), Vmcs::read<mword> (Vmcs::GUEST_BASE_FS), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_FS), Vmcs::read<uint32> (Vmcs::GUEST_AR_FS));
        gs.set_vmx (Vmcs::read<uint16> (Vmcs::GUEST_SEL_GS), Vmcs::read<mword> (Vmcs::GUEST_BASE_GS), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_GS), Vmcs::read<uint32> (Vmcs::GUEST_AR_GS));
    }

    if (m & Mtd_arch::CS_SS) {
        cs.set_vmx (Vmcs::read<uint16> (Vmcs::GUEST_SEL_CS), Vmcs::read<mword> (Vmcs::GUEST_BASE_CS), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_CS), Vmcs::read<uint32> (Vmcs::GUEST_AR_CS));
        ss.set_vmx (Vmcs::read<uint16> (Vmcs::GUEST_SEL_SS), Vmcs::read<mword> (Vmcs::GUEST_BASE_SS), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_SS), Vmcs::read<uint32> (Vmcs::GUEST_AR_SS));
    }

    if (m & Mtd_arch::TR)
        tr.set_vmx (Vmcs::read<uint16> (Vmcs::GUEST_SEL_TR), Vmcs::read<mword> (Vmcs::GUEST_BASE_TR), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_TR), Vmcs::read<uint32> (Vmcs::GUEST_AR_TR));

    if (m & Mtd_arch::LDTR)
        ld.set_vmx (Vmcs::read<uint16> (Vmcs::GUEST_SEL_LDTR), Vmcs::read<mword> (Vmcs::GUEST_BASE_LDTR), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_LDTR), Vmcs::read<uint32> (Vmcs::GUEST_AR_LDTR));

    if (m & Mtd_arch::GDTR)
        gd.set_vmx (0, Vmcs::read<mword> (Vmcs::GUEST_BASE_GDTR), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_GDTR), 0);

    if (m & Mtd_arch::IDTR)
        id.set_vmx (0, Vmcs::read<mword> (Vmcs::GUEST_BASE_IDTR), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_IDTR), 0);

    if (m & Mtd_arch::CR) {
        cr0 = regs->get_cr0<Vmcs>();
        cr2 = regs->get_cr2<Vmcs>();
        cr3 = regs->get_cr3<Vmcs>();
        cr4 = regs->get_cr4<Vmcs>();
        pdpte[0] = Vmcs::read<uint64> (Vmcs::GUEST_PDPTE0);
        pdpte[1] = Vmcs::read<uint64> (Vmcs::GUEST_PDPTE1);
        pdpte[2] = Vmcs::read<uint64> (Vmcs::GUEST_PDPTE2);
        pdpte[3] = Vmcs::read<uint64> (Vmcs::GUEST_PDPTE3);
    }

    if (m & Mtd_arch::DR)
        dr7 = Vmcs::read<mword> (Vmcs::GUEST_DR7);

    if (m & Mtd_arch::SYSENTER) {
        sysenter_cs  = Vmcs::read<uint32> (Vmcs::GUEST_SYSENTER_CS);
        sysenter_rsp = Vmcs::read<mword>  (Vmcs::GUEST_SYSENTER_ESP);
        sysenter_rip = Vmcs::read<mword>  (Vmcs::GUEST_SYSENTER_EIP);
    }

    if (m & Mtd_arch::QUAL) {
        qual[0] = Vmcs::read<mword>  (Vmcs::EXI_QUALIFICATION);
        qual[1] = Vmcs::read<uint64> (Vmcs::INFO_PHYS_ADDR);
    }

    if (m & Mtd_arch::INJ) {
        if (regs->dst_portal == 33 || regs->dst_portal == NUM_VMI - 1) {
            intr_info  = Vmcs::read<uint32> (Vmcs::ENT_INTR_INFO);
            intr_error = Vmcs::read<uint32> (Vmcs::ENT_INTR_ERROR);
        } else {
            intr_info  = Vmcs::read<uint32> (Vmcs::IDT_VECT_INFO);
            intr_error = Vmcs::read<uint32> (Vmcs::IDT_VECT_ERROR);
        }
    }

    if (m & Mtd_arch::STA) {
        intr_state = Vmcs::read<uint32> (Vmcs::GUEST_INTR_STATE);
        actv_state = Vmcs::read<uint32> (Vmcs::GUEST_ACTV_STATE);
    }

    if (m & Mtd_arch::EFER)
        efer = Vmcs::read<uint64> (Vmcs::GUEST_EFER);

    barrier();
    mtd = m;
    items = sizeof (Utcb_data) / sizeof (mword);

    return m & Mtd_arch::FPU;
}

bool Utcb::save_vmx (Cpu_regs *regs)
{
    regs->vmcs->make_current();

    if (mtd & Mtd_arch::GPR_0_7) {
        regs->rax = rax;
        regs->rcx = rcx;
        regs->rdx = rdx;
        regs->rbx = rbx;
        Vmcs::write (Vmcs::GUEST_RSP, rsp);
        regs->rbp = rbp;
        regs->rsi = rsi;
        regs->rdi = rdi;
    }

    if (mtd & Mtd_arch::GPR_8_15) {
        regs->r8  = r8;
        regs->r9  = r9;
        regs->r10 = r10;
        regs->r11 = r11;
        regs->r12 = r12;
        regs->r13 = r13;
        regs->r14 = r14;
        regs->r15 = r15;
    }

    if (mtd & Mtd_arch::RIP) {
        Vmcs::write (Vmcs::GUEST_RIP, rip);
        Vmcs::write (Vmcs::ENT_INST_LEN, static_cast<uint32> (inst_len));
    }

    if (mtd & Mtd_arch::RFLAGS)
        Vmcs::write (Vmcs::GUEST_RFLAGS, rflags);

    if (mtd & Mtd_arch::DS_ES) {
        Vmcs::write (Vmcs::GUEST_SEL_DS,   ds.sel);
        Vmcs::write (Vmcs::GUEST_BASE_DS,  static_cast<mword>(ds.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_DS, ds.limit);
        Vmcs::write (Vmcs::GUEST_AR_DS,   (ds.ar << 4 & 0x1f000) | (ds.ar & 0xff));
        Vmcs::write (Vmcs::GUEST_SEL_ES,   es.sel);
        Vmcs::write (Vmcs::GUEST_BASE_ES,  static_cast<mword>(es.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_ES, es.limit);
        Vmcs::write (Vmcs::GUEST_AR_ES,   (es.ar << 4 & 0x1f000) | (es.ar & 0xff));
    }

    if (mtd & Mtd_arch::FS_GS) {
        Vmcs::write (Vmcs::GUEST_SEL_FS,   fs.sel);
        Vmcs::write (Vmcs::GUEST_BASE_FS,  static_cast<mword>(fs.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_FS, fs.limit);
        Vmcs::write (Vmcs::GUEST_AR_FS,   (fs.ar << 4 & 0x1f000) | (fs.ar & 0xff));
        Vmcs::write (Vmcs::GUEST_SEL_GS,   gs.sel);
        Vmcs::write (Vmcs::GUEST_BASE_GS,  static_cast<mword>(gs.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_GS, gs.limit);
        Vmcs::write (Vmcs::GUEST_AR_GS,   (gs.ar << 4 & 0x1f000) | (gs.ar & 0xff));
    }

    if (mtd & Mtd_arch::CS_SS) {
        Vmcs::write (Vmcs::GUEST_SEL_CS,   cs.sel);
        Vmcs::write (Vmcs::GUEST_BASE_CS,  static_cast<mword>(cs.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_CS, cs.limit);
        Vmcs::write (Vmcs::GUEST_AR_CS,   (cs.ar << 4 & 0x1f000) | (cs.ar & 0xff));
        Vmcs::write (Vmcs::GUEST_SEL_SS,   ss.sel);
        Vmcs::write (Vmcs::GUEST_BASE_SS,  static_cast<mword>(ss.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_SS, ss.limit);
        Vmcs::write (Vmcs::GUEST_AR_SS,   (ss.ar << 4 & 0x1f000) | (ss.ar & 0xff));
    }

    if (mtd & Mtd_arch::TR) {
        Vmcs::write (Vmcs::GUEST_SEL_TR,     tr.sel);
        Vmcs::write (Vmcs::GUEST_BASE_TR,    static_cast<mword>(tr.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_TR,   tr.limit);
        Vmcs::write (Vmcs::GUEST_AR_TR,     (tr.ar << 4 & 0x1f000) | (tr.ar & 0xff));
    }

    if (mtd & Mtd_arch::LDTR) {
        Vmcs::write (Vmcs::GUEST_SEL_LDTR,   ld.sel);
        Vmcs::write (Vmcs::GUEST_BASE_LDTR,  static_cast<mword>(ld.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_LDTR, ld.limit);
        Vmcs::write (Vmcs::GUEST_AR_LDTR,   (ld.ar << 4 & 0x1f000) | (ld.ar & 0xff));
    }

    if (mtd & Mtd_arch::GDTR) {
        Vmcs::write (Vmcs::GUEST_BASE_GDTR,  static_cast<mword>(gd.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_GDTR, gd.limit);
    }

    if (mtd & Mtd_arch::IDTR) {
        Vmcs::write (Vmcs::GUEST_BASE_IDTR,  static_cast<mword>(id.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_IDTR, id.limit);
    }

    if (mtd & Mtd_arch::CR) {
        regs->set_cr0<Vmcs> (cr0);
        regs->set_cr2<Vmcs> (cr2);
        regs->set_cr3<Vmcs> (cr3);
        regs->set_cr4<Vmcs> (cr4);
        Vmcs::write (Vmcs::GUEST_PDPTE0, pdpte[0]);
        Vmcs::write (Vmcs::GUEST_PDPTE1, pdpte[1]);
        Vmcs::write (Vmcs::GUEST_PDPTE2, pdpte[2]);
        Vmcs::write (Vmcs::GUEST_PDPTE3, pdpte[3]);
    }

    if (mtd & Mtd_arch::DR)
        Vmcs::write (Vmcs::GUEST_DR7, dr7);

    if (mtd & Mtd_arch::SYSENTER) {
        Vmcs::write (Vmcs::GUEST_SYSENTER_CS,  sysenter_cs);
        Vmcs::write (Vmcs::GUEST_SYSENTER_ESP, sysenter_rsp);
        Vmcs::write (Vmcs::GUEST_SYSENTER_EIP, sysenter_rip);
    }

    if (mtd & Mtd_arch::CTRL) {
        regs->set_cpu_ctrl0<Vmcs> (ctrl[0]);
        regs->set_cpu_ctrl1<Vmcs> (ctrl[1]);
    }

    if (mtd & Mtd_arch::INJ) {

        uint32 val = Vmcs::read<uint32> (Vmcs::CPU_EXEC_CTRL0);

        if (intr_info & 0x1000)
            val |=  Vmcs::CPU_INTR_WINDOW;
        else
            val &= ~Vmcs::CPU_INTR_WINDOW;

        if (intr_info & 0x2000)
            val |=  Vmcs::CPU_NMI_WINDOW;
        else
            val &= ~Vmcs::CPU_NMI_WINDOW;

        regs->set_cpu_ctrl0<Vmcs> (val);

        Vmcs::write (Vmcs::ENT_INTR_INFO,  intr_info & ~0x3000);
        Vmcs::write (Vmcs::ENT_INTR_ERROR, intr_error);
    }

    if (mtd & Mtd_arch::STA) {
        Vmcs::write (Vmcs::GUEST_INTR_STATE, intr_state);
        Vmcs::write (Vmcs::GUEST_ACTV_STATE, actv_state);
    }

    if (mtd & Mtd_arch::EFER)
        regs->write_efer<Vmcs> (efer);

    return mtd & Mtd_arch::FPU;
}

bool Utcb::load_svm (Cpu_regs *regs)
{
    Vmcb *const vmcb = regs->vmcb;

    mword m = regs->mtd;

    if (m & Mtd_arch::GPR_0_7) {
        rax = static_cast<mword>(vmcb->rax);
        rcx = regs->rcx;
        rdx = regs->rdx;
        rbx = regs->rbx;
        rsp = static_cast<mword>(vmcb->rsp);
        rbp = regs->rbp;
        rsi = regs->rsi;
        rdi = regs->rdi;
    }

    if (m & Mtd_arch::GPR_8_15) {
        r8  = regs->r8;
        r9  = regs->r9;
        r10 = regs->r10;
        r11 = regs->r11;
        r12 = regs->r12;
        r13 = regs->r13;
        r14 = regs->r14;
        r15 = regs->r15;
    }

    if (m & Mtd_arch::RIP)
        rip = static_cast<mword>(vmcb->rip);

    if (m & Mtd_arch::RFLAGS)
        rflags = static_cast<mword>(vmcb->rflags);

    if (m & Mtd_arch::DS_ES) {
        ds = vmcb->ds;
        es = vmcb->es;
    }

    if (m & Mtd_arch::FS_GS) {
        fs = vmcb->fs;
        gs = vmcb->gs;
    }

    if (m & Mtd_arch::CS_SS) {
        cs = vmcb->cs;
        ss = vmcb->ss;
    }

    if (m & Mtd_arch::TR)
        tr = vmcb->tr;

    if (m & Mtd_arch::LDTR)
        ld = vmcb->ldtr;

    if (m & Mtd_arch::GDTR)
        gd = vmcb->gdtr;

    if (m & Mtd_arch::IDTR)
        id = vmcb->idtr;

    if (m & Mtd_arch::CR) {
        cr0 = regs->get_cr0<Vmcb>();
        cr2 = regs->get_cr2<Vmcb>();
        cr3 = regs->get_cr3<Vmcb>();
        cr4 = regs->get_cr4<Vmcb>();
    }

    if (m & Mtd_arch::DR)
        dr7 = static_cast<mword>(vmcb->dr7);

    if (m & Mtd_arch::SYSENTER) {
        sysenter_cs  = static_cast<mword>(vmcb->sysenter_cs);
        sysenter_rsp = static_cast<mword>(vmcb->sysenter_esp);
        sysenter_rip = static_cast<mword>(vmcb->sysenter_eip);
    }

    if (m & Mtd_arch::QUAL) {
        qual[0] = vmcb->exitinfo1;
        qual[1] = vmcb->exitinfo2;
    }

    if (m & Mtd_arch::INJ) {
        if (regs->dst_portal == NUM_VMI - 3 || regs->dst_portal == NUM_VMI - 1)
            inj = vmcb->inj_control;
        else
            inj = vmcb->exitintinfo;
    }

    if (m & Mtd_arch::STA) {
        intr_state = static_cast<uint32>(vmcb->int_shadow);
        actv_state = 0;
    }

    if (m & Mtd_arch::EFER)
        efer = vmcb->efer;

    barrier();
    mtd = m;
    items = sizeof (Utcb_data) / sizeof (mword);

    return m & Mtd_arch::FPU;
}

bool Utcb::save_svm (Cpu_regs *regs)
{
    Vmcb * const vmcb = regs->vmcb;

    if (mtd & Mtd_arch::GPR_0_7) {
        vmcb->rax = rax;
        regs->rcx = rcx;
        regs->rdx = rdx;
        regs->rbx = rbx;
        vmcb->rsp = rsp;
        regs->rbp = rbp;
        regs->rsi = rsi;
        regs->rdi = rdi;
    }

    if (mtd & Mtd_arch::GPR_8_15) {
        regs->r8  = r8;
        regs->r9  = r9;
        regs->r10 = r10;
        regs->r11 = r11;
        regs->r12 = r12;
        regs->r13 = r13;
        regs->r14 = r14;
        regs->r15 = r15;
    }

    if (mtd & Mtd_arch::RIP)
        vmcb->rip = rip;

    if (mtd & Mtd_arch::RFLAGS)
        vmcb->rflags = rflags;

    if (mtd & Mtd_arch::DS_ES) {
        vmcb->ds = ds;
        vmcb->es = es;
    }

    if (mtd & Mtd_arch::FS_GS) {
        vmcb->fs = fs;
        vmcb->gs = gs;
    }

    if (mtd & Mtd_arch::CS_SS) {
        vmcb->cs = cs;
        vmcb->ss = ss;
    }

    if (mtd & Mtd_arch::TR)
        vmcb->tr = tr;

    if (mtd & Mtd_arch::LDTR)
        vmcb->ldtr = ld;

    if (mtd & Mtd_arch::GDTR)
        vmcb->gdtr = gd;

    if (mtd & Mtd_arch::IDTR)
        vmcb->idtr = id;

    if (mtd & Mtd_arch::CR) {
        regs->set_cr0<Vmcb> (cr0);
        regs->set_cr2<Vmcb> (cr2);
        regs->set_cr3<Vmcb> (cr3);
        regs->set_cr4<Vmcb> (cr4);
    }

    if (mtd & Mtd_arch::DR)
        vmcb->dr7 = dr7;

    if (mtd & Mtd_arch::SYSENTER) {
        vmcb->sysenter_cs  = sysenter_cs;
        vmcb->sysenter_esp = sysenter_rsp;
        vmcb->sysenter_eip = sysenter_rip;
    }

    if (mtd & Mtd_arch::CTRL) {
        regs->set_cpu_ctrl0<Vmcb> (ctrl[0]);
        regs->set_cpu_ctrl1<Vmcb> (ctrl[1]);
    }

    if (mtd & Mtd_arch::INJ) {

        if (intr_info & 0x1000) {
            vmcb->int_control      |=  (1ul << 8 | 1ul << 20);
            vmcb->intercept_cpu[0] |=  Vmcb::CPU_VINTR;
        } else {
            vmcb->int_control      &= ~(1ul << 8 | 1ul << 20);
            vmcb->intercept_cpu[0] &= ~Vmcb::CPU_VINTR;
        }

        vmcb->inj_control = inj & ~0x3000;
    }

    if (mtd & Mtd_arch::STA)
        vmcb->int_shadow = intr_state;

    if (mtd & Mtd_arch::EFER)
        regs->write_efer<Vmcb> (efer);

    return mtd & Mtd_arch::FPU;
}
