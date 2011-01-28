/*
 * User Thread Control Block (UTCB)
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

#include "barrier.h"
#include "mtd.h"
#include "regs.h"
#include "svm.h"
#include "vmx.h"

void Utcb::load_exc (Cpu_regs *regs)
{
    mword m = regs->mtd;

    if (m & Mtd::GPR_ACDB) {
        rax = regs->eax;
        rcx = regs->ecx;
        rdx = regs->edx;
        rbx = regs->ebx;
    }

    if (m & Mtd::GPR_BSD) {
        rbp = regs->ebp;
        rsi = regs->esi;
        rdi = regs->edi;
    }

    if (m & Mtd::RSP)
        rsp = regs->esp;

    if (m & Mtd::RIP_LEN)
        rip = regs->eip;

    if (m & Mtd::RFLAGS)
        rflags = regs->efl;

    if (m & Mtd::QUAL) {
        qual[0] = regs->err;
        qual[1] = regs->cr2;
    }

    barrier();
    mtd = m;
    items = sizeof (Utcb_data) / sizeof (mword);
}

void Utcb::save_exc (Cpu_regs *regs)
{
    if (mtd & Mtd::GPR_ACDB) {
        regs->eax = rax;
        regs->ecx = rcx;
        regs->edx = rdx;
        regs->ebx = rbx;
    }

    if (mtd & Mtd::GPR_BSD) {
        regs->ebp = rbp;
        regs->esi = rsi;
        regs->edi = rdi;
    }

    if (mtd & Mtd::RSP)
        regs->esp = rsp;

    if (mtd & Mtd::RIP_LEN)
        regs->eip = rip;

    if (mtd & Mtd::RFLAGS)
        regs->efl = (rflags & ~(Cpu::EFL_VIP | Cpu::EFL_VIF | Cpu::EFL_VM | Cpu::EFL_RF | Cpu::EFL_IOPL)) | Cpu::EFL_IF;
}

void Utcb::load_vmx (Cpu_regs *regs)
{
    mword m = regs->mtd;

    if (m & Mtd::GPR_ACDB) {
        rax = regs->eax;
        rcx = regs->ecx;
        rdx = regs->edx;
        rbx = regs->ebx;
    }

    if (m & Mtd::GPR_BSD) {
        rbp = regs->ebp;
        rsi = regs->esi;
        rdi = regs->edi;
    }

    regs->vmcs->make_current();

    if (m & Mtd::RSP)
        rsp = Vmcs::read (Vmcs::GUEST_RSP);

    if (m & Mtd::RIP_LEN) {
        rip      = Vmcs::read (Vmcs::GUEST_RIP);
        inst_len = Vmcs::read (Vmcs::EXI_INST_LEN);
    }

    if (m & Mtd::RFLAGS)
        rflags = Vmcs::read (Vmcs::GUEST_RFLAGS);

    if (m & Mtd::DS_ES) {
        ds.set_vmx (Vmcs::read (Vmcs::GUEST_SEL_DS), Vmcs::read (Vmcs::GUEST_BASE_DS), Vmcs::read (Vmcs::GUEST_LIMIT_DS), Vmcs::read (Vmcs::GUEST_AR_DS));
        es.set_vmx (Vmcs::read (Vmcs::GUEST_SEL_ES), Vmcs::read (Vmcs::GUEST_BASE_ES), Vmcs::read (Vmcs::GUEST_LIMIT_ES), Vmcs::read (Vmcs::GUEST_AR_ES));
    }

    if (m & Mtd::FS_GS) {
        fs.set_vmx (Vmcs::read (Vmcs::GUEST_SEL_FS), Vmcs::read (Vmcs::GUEST_BASE_FS), Vmcs::read (Vmcs::GUEST_LIMIT_FS), Vmcs::read (Vmcs::GUEST_AR_FS));
        gs.set_vmx (Vmcs::read (Vmcs::GUEST_SEL_GS), Vmcs::read (Vmcs::GUEST_BASE_GS), Vmcs::read (Vmcs::GUEST_LIMIT_GS), Vmcs::read (Vmcs::GUEST_AR_GS));
    }

    if (m & Mtd::CS_SS) {
        cs.set_vmx (Vmcs::read (Vmcs::GUEST_SEL_CS), Vmcs::read (Vmcs::GUEST_BASE_CS), Vmcs::read (Vmcs::GUEST_LIMIT_CS), Vmcs::read (Vmcs::GUEST_AR_CS));
        ss.set_vmx (Vmcs::read (Vmcs::GUEST_SEL_SS), Vmcs::read (Vmcs::GUEST_BASE_SS), Vmcs::read (Vmcs::GUEST_LIMIT_SS), Vmcs::read (Vmcs::GUEST_AR_SS));
    }

    if (m & Mtd::TR)
        tr.set_vmx (Vmcs::read (Vmcs::GUEST_SEL_TR), Vmcs::read (Vmcs::GUEST_BASE_TR), Vmcs::read (Vmcs::GUEST_LIMIT_TR), Vmcs::read (Vmcs::GUEST_AR_TR));

    if (m & Mtd::LDTR)
        ld.set_vmx (Vmcs::read (Vmcs::GUEST_SEL_LDTR), Vmcs::read (Vmcs::GUEST_BASE_LDTR), Vmcs::read (Vmcs::GUEST_LIMIT_LDTR), Vmcs::read (Vmcs::GUEST_AR_LDTR));

    if (m & Mtd::GDTR)
        gd.set_vmx (0, Vmcs::read (Vmcs::GUEST_BASE_GDTR), Vmcs::read (Vmcs::GUEST_LIMIT_GDTR), 0);

    if (m & Mtd::IDTR)
        id.set_vmx (0, Vmcs::read (Vmcs::GUEST_BASE_IDTR), Vmcs::read (Vmcs::GUEST_LIMIT_IDTR), 0);

    if (m & Mtd::CR) {
        cr0 = regs->read_cr<Vmcs> (0);
        cr2 = regs->read_cr<Vmcs> (2);
        cr3 = regs->read_cr<Vmcs> (3);
        cr4 = regs->read_cr<Vmcs> (4);
    }

    if (m & Mtd::DR)
        dr7 = Vmcs::read (Vmcs::GUEST_DR7);

    if (m & Mtd::SYSENTER) {
        sysenter_cs  = Vmcs::read (Vmcs::GUEST_SYSENTER_CS);
        sysenter_rsp = Vmcs::read (Vmcs::GUEST_SYSENTER_ESP);
        sysenter_rip = Vmcs::read (Vmcs::GUEST_SYSENTER_EIP);
    }

    if (m & Mtd::QUAL) {
        if (regs->dst_portal == 48 && !regs->nst_on) {
            qual[0] = regs->nst_error;
            qual[1] = regs->nst_fault;
        } else {
            qual[0] = Vmcs::read (Vmcs::EXI_QUALIFICATION);
            qual[1] = Vmcs::read (Vmcs::INFO_PHYS_ADDR);
        }
    }

    if (m & Mtd::INJ) {
        if (regs->dst_portal == 33 || regs->dst_portal == NUM_VMI - 1) {
            intr_info  = static_cast<uint32>(Vmcs::read (Vmcs::ENT_INTR_INFO));
            intr_error = static_cast<uint32>(Vmcs::read (Vmcs::ENT_INTR_ERROR));
        } else {
            intr_info  = static_cast<uint32>(Vmcs::read (Vmcs::IDT_VECT_INFO));
            intr_error = static_cast<uint32>(Vmcs::read (Vmcs::IDT_VECT_ERROR));
        }
    }

    if (m & Mtd::STA) {
        intr_state = static_cast<uint32>(Vmcs::read (Vmcs::GUEST_INTR_STATE));
        actv_state = static_cast<uint32>(Vmcs::read (Vmcs::GUEST_ACTV_STATE));
    }

    if (m & Mtd::TSC)
        tsc = regs->tsc_offset;

    barrier();
    mtd = m;
    items = sizeof (Utcb_data) / sizeof (mword);
}

void Utcb::save_vmx (Cpu_regs *regs)
{
    if (mtd & Mtd::GPR_ACDB) {
        regs->eax = rax;
        regs->ecx = rcx;
        regs->edx = rdx;
        regs->ebx = rbx;
    }

    if (mtd & Mtd::GPR_BSD) {
        regs->ebp = rbp;
        regs->esi = rsi;
        regs->edi = rdi;
    }

    regs->vmcs->make_current();

    if (mtd & Mtd::RSP)
        Vmcs::write (Vmcs::GUEST_RSP, rsp);
    if (mtd & Mtd::RIP_LEN)
        Vmcs::write (Vmcs::GUEST_RIP, rip);
    if (mtd & Mtd::RFLAGS)
        Vmcs::write (Vmcs::GUEST_RFLAGS, rflags);

    if (mtd & Mtd::DS_ES) {
        Vmcs::write (Vmcs::GUEST_SEL_DS,   ds.sel);
        Vmcs::write (Vmcs::GUEST_BASE_DS,  ds.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_DS, ds.limit);
        Vmcs::write (Vmcs::GUEST_AR_DS,   (ds.ar << 4 & 0x1f000) | (ds.ar & 0xff));
        Vmcs::write (Vmcs::GUEST_SEL_ES,   es.sel);
        Vmcs::write (Vmcs::GUEST_BASE_ES,  es.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_ES, es.limit);
        Vmcs::write (Vmcs::GUEST_AR_ES,   (es.ar << 4 & 0x1f000) | (es.ar & 0xff));
    }

    if (mtd & Mtd::FS_GS) {
        Vmcs::write (Vmcs::GUEST_SEL_FS,   fs.sel);
        Vmcs::write (Vmcs::GUEST_BASE_FS,  fs.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_FS, fs.limit);
        Vmcs::write (Vmcs::GUEST_AR_FS,   (fs.ar << 4 & 0x1f000) | (fs.ar & 0xff));
        Vmcs::write (Vmcs::GUEST_SEL_GS,   gs.sel);
        Vmcs::write (Vmcs::GUEST_BASE_GS,  gs.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_GS, gs.limit);
        Vmcs::write (Vmcs::GUEST_AR_GS,   (gs.ar << 4 & 0x1f000) | (gs.ar & 0xff));
    }

    if (mtd & Mtd::CS_SS) {
        Vmcs::write (Vmcs::GUEST_SEL_CS,   cs.sel);
        Vmcs::write (Vmcs::GUEST_BASE_CS,  cs.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_CS, cs.limit);
        Vmcs::write (Vmcs::GUEST_AR_CS,   (cs.ar << 4 & 0x1f000) | (cs.ar & 0xff));
        Vmcs::write (Vmcs::GUEST_SEL_SS,   ss.sel);
        Vmcs::write (Vmcs::GUEST_BASE_SS,  ss.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_SS, ss.limit);
        Vmcs::write (Vmcs::GUEST_AR_SS,   (ss.ar << 4 & 0x1f000) | (ss.ar & 0xff));
    }

    if (mtd & Mtd::TR) {
        Vmcs::write (Vmcs::GUEST_SEL_TR,     tr.sel);
        Vmcs::write (Vmcs::GUEST_BASE_TR,    tr.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_TR,   tr.limit);
        Vmcs::write (Vmcs::GUEST_AR_TR,     (tr.ar << 4 & 0x1f000) | (tr.ar & 0xff));
    }

    if (mtd & Mtd::LDTR) {
        Vmcs::write (Vmcs::GUEST_SEL_LDTR,   ld.sel);
        Vmcs::write (Vmcs::GUEST_BASE_LDTR,  ld.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_LDTR, ld.limit);
        Vmcs::write (Vmcs::GUEST_AR_LDTR,   (ld.ar << 4 & 0x1f000) | (ld.ar & 0xff));
    }

    if (mtd & Mtd::GDTR) {
        Vmcs::write (Vmcs::GUEST_BASE_GDTR,  gd.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_GDTR, gd.limit);
    }

    if (mtd & Mtd::IDTR) {
        Vmcs::write (Vmcs::GUEST_BASE_IDTR,  id.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_IDTR, id.limit);
    }

    if (mtd & Mtd::CR) {
        regs->write_cr<Vmcs> (0, cr0);
        regs->write_cr<Vmcs> (2, cr2);
        regs->write_cr<Vmcs> (3, cr3);
        regs->write_cr<Vmcs> (4, cr4);
    }

    if (mtd & Mtd::DR)
        Vmcs::write (Vmcs::GUEST_DR7, dr7);

    if (mtd & Mtd::SYSENTER) {
        Vmcs::write (Vmcs::GUEST_SYSENTER_CS,  sysenter_cs);
        Vmcs::write (Vmcs::GUEST_SYSENTER_ESP, sysenter_rsp);
        Vmcs::write (Vmcs::GUEST_SYSENTER_EIP, sysenter_rip);
    }

    if (mtd & Mtd::CTRL) {
        regs->vmx_set_cpu_ctrl0 (ctrl[0]);
        regs->vmx_set_cpu_ctrl1 (ctrl[1]);
    }

    if (mtd & Mtd::INJ) {

        uint32 val = static_cast<uint32>(Vmcs::read (Vmcs::CPU_EXEC_CTRL0));

        if (intr_info & 0x1000)
            val |=  Vmcs::CPU_INTR_WINDOW;
        else
            val &= ~Vmcs::CPU_INTR_WINDOW;

        Vmcs::write (Vmcs::CPU_EXEC_CTRL0, val);
        Vmcs::write (Vmcs::ENT_INTR_INFO,  intr_info & ~0x1000);
        Vmcs::write (Vmcs::ENT_INTR_ERROR, intr_error);
    }

    if (mtd & Mtd::STA) {
        Vmcs::write (Vmcs::GUEST_INTR_STATE, intr_state);
        Vmcs::write (Vmcs::GUEST_ACTV_STATE, actv_state);
    }

    if (mtd & Mtd::TSC)
        regs->add_tsc_offset (tsc);
}

void Utcb::load_svm (Cpu_regs *regs)
{
    Vmcb *const vmcb = regs->vmcb;

    mword m = regs->mtd;

    if (m & Mtd::GPR_ACDB) {
        rax = static_cast<mword>(vmcb->rax);
        rcx = regs->ecx;
        rdx = regs->edx;
        rbx = regs->ebx;
    }

    if (m & Mtd::GPR_BSD) {
        rbp = regs->ebp;
        rsi = regs->esi;
        rdi = regs->edi;
    }

    if (m & Mtd::RSP)
        rsp = static_cast<mword>(vmcb->rsp);

    if (m & Mtd::RIP_LEN)
        rip = static_cast<mword>(vmcb->rip);

    if (m & Mtd::RFLAGS)
        rflags = static_cast<mword>(vmcb->rflags);

    if (m & Mtd::DS_ES) {
        ds = vmcb->ds;
        es = vmcb->es;
    }

    if (m & Mtd::FS_GS) {
        fs = vmcb->fs;
        gs = vmcb->gs;
    }

    if (m & Mtd::CS_SS) {
        cs = vmcb->cs;
        ss = vmcb->ss;
    }

    if (m & Mtd::TR)
        tr = vmcb->tr;

    if (m & Mtd::LDTR)
        ld = vmcb->ldtr;

    if (m & Mtd::GDTR)
        gd = vmcb->gdtr;

    if (m & Mtd::IDTR)
        id = vmcb->idtr;

    if (m & Mtd::CR) {
        cr0 = regs->read_cr<Vmcb> (0);
        cr2 = regs->read_cr<Vmcb> (2);
        cr3 = regs->read_cr<Vmcb> (3);
        cr4 = regs->read_cr<Vmcb> (4);
    }

    if (m & Mtd::DR)
        dr7 = static_cast<mword>(vmcb->dr7);

    if (m & Mtd::SYSENTER) {
        sysenter_cs  = static_cast<mword>(vmcb->sysenter_cs);
        sysenter_rsp = static_cast<mword>(vmcb->sysenter_esp);
        sysenter_rip = static_cast<mword>(vmcb->sysenter_eip);
    }

    if (m & Mtd::QUAL) {
        if (regs->dst_portal == NUM_VMI - 4 && !regs->nst_on) {
            qual[0] = regs->nst_error;
            qual[1] = regs->nst_fault;
        } else {
            qual[0] = vmcb->exitinfo1;
            qual[1] = vmcb->exitinfo2;
        }
    }

    if (m & Mtd::INJ) {
        if (regs->dst_portal == NUM_VMI - 3 || regs->dst_portal == NUM_VMI - 1)
            inj = vmcb->inj_control;
        else
            inj = vmcb->exitintinfo;
    }

    if (m & Mtd::STA) {
        intr_state = static_cast<uint32>(vmcb->int_shadow);
        actv_state = 0;
    }

    if (m & Mtd::TSC)
        tsc = regs->tsc_offset;

    barrier();
    mtd = m;
    items = sizeof (Utcb_data) / sizeof (mword);
}

void Utcb::save_svm (Cpu_regs *regs)
{
    Vmcb * const vmcb = regs->vmcb;

    if (mtd & Mtd::GPR_ACDB) {
        vmcb->rax = rax;
        regs->ecx = rcx;
        regs->edx = rdx;
        regs->ebx = rbx;
    }

    if (mtd & Mtd::GPR_BSD) {
        regs->ebp = rbp;
        regs->esi = rsi;
        regs->edi = rdi;
    }

    if (mtd & Mtd::RSP)
        vmcb->rsp = rsp;

    if (mtd & Mtd::RIP_LEN)
        vmcb->rip = rip;

    if (mtd & Mtd::RFLAGS)
        vmcb->rflags = rflags;

    if (mtd & Mtd::DS_ES) {
        vmcb->ds = ds;
        vmcb->es = es;
    }

    if (mtd & Mtd::FS_GS) {
        vmcb->fs = fs;
        vmcb->gs = gs;
    }

    if (mtd & Mtd::CS_SS) {
        vmcb->cs = cs;
        vmcb->ss = ss;
    }

    if (mtd & Mtd::TR)
        vmcb->tr = tr;

    if (mtd & Mtd::LDTR)
        vmcb->ldtr = ld;

    if (mtd & Mtd::GDTR)
        vmcb->gdtr = gd;

    if (mtd & Mtd::IDTR)
        vmcb->idtr = id;

    if (mtd & Mtd::CR) {
        regs->write_cr<Vmcb> (0, cr0);
        regs->write_cr<Vmcb> (2, cr2);
        regs->write_cr<Vmcb> (3, cr3);
        regs->write_cr<Vmcb> (4, cr4);
    }

    if (mtd & Mtd::DR)
        vmcb->dr7 = dr7;

    if (mtd & Mtd::SYSENTER) {
        vmcb->sysenter_cs  = sysenter_cs;
        vmcb->sysenter_esp = sysenter_rsp;
        vmcb->sysenter_eip = sysenter_rip;
    }

    if (mtd & Mtd::CTRL) {
        regs->svm_set_cpu_ctrl0 (ctrl[0]);
        regs->svm_set_cpu_ctrl1 (ctrl[1]);
    }

    if (mtd & Mtd::INJ) {

        if (intr_info & 0x1000) {
            vmcb->int_control      |=  (1ul << 8 | 1ul << 20);
            vmcb->intercept_cpu[0] |=  Vmcb::CPU_VINTR;
        } else {
            vmcb->int_control      &= ~(1ul << 8 | 1ul << 20);
            vmcb->intercept_cpu[0] &= ~Vmcb::CPU_VINTR;
        }

        vmcb->inj_control = inj;
    }

    if (mtd & Mtd::STA)
        vmcb->int_shadow = intr_state;

    if (mtd & Mtd::TSC)
        regs->add_tsc_offset (tsc);
}
