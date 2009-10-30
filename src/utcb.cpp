/*
 * User Thread Control Block (UTCB)
 *
 * Copyright (C) 2007-2009, Udo Steinberg <udo@hypervisor.org>
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

#include "mtd.h"
#include "regs.h"
#include "svm.h"
#include "utcb.h"
#include "vmx.h"

void Utcb::load_exc (Exc_regs *regs, Mtd mtd)
{
    mtr = mtd;

    if (mtd.xfer (Mtd::GPR_ACDB)) {
        rax = regs->eax;
        rcx = regs->ecx;
        rdx = regs->edx;
        rbx = regs->ebx;
    }

    if (mtd.xfer (Mtd::GPR_BSD)) {
        rbp = regs->ebp;
        rsi = regs->esi;
        rdi = regs->edi;
    }

    if (mtd.xfer (Mtd::RSP))
        rsp = regs->esp;

    if (mtd.xfer (Mtd::RIP_LEN))
        rip = regs->eip;

    if (mtd.xfer (Mtd::RFLAGS))
        rflags = regs->efl;

    if (mtd.xfer (Mtd::QUAL)) {
        qual[0] = regs->err;
        qual[1] = regs->cr2;
    }
}

mword *Utcb::save_exc (Exc_regs *regs, Mtd mtd)
{
    if (mtd.xfer (Mtd::GPR_ACDB)) {
        regs->eax = rax;
        regs->ecx = rcx;
        regs->edx = rdx;
        regs->ebx = rbx;
    }

    if (mtd.xfer (Mtd::GPR_BSD)) {
        regs->ebp = rbp;
        regs->esi = rsi;
        regs->edi = rdi;
    }

    if (mtd.xfer (Mtd::RSP))
        regs->esp = rsp;

    if (mtd.xfer (Mtd::RIP_LEN))
        regs->eip = rip;

    if (mtd.xfer (Mtd::RFLAGS))
        regs->efl = (rflags & ~(Cpu::EFL_VIP | Cpu::EFL_VIF | Cpu::EFL_VM | Cpu::EFL_RF | Cpu::EFL_IOPL)) | Cpu::EFL_IF;

    return item;
}

void Utcb::load_vmx (Exc_regs *regs, Mtd mtd)
{
    mtr = mtd;

    if (mtd.xfer (Mtd::GPR_ACDB)) {
        rax = regs->eax;
        rcx = regs->ecx;
        rdx = regs->edx;
        rbx = regs->ebx;
    }

    if (mtd.xfer (Mtd::GPR_BSD)) {
        rbp = regs->ebp;
        rsi = regs->esi;
        rdi = regs->edi;
    }

    regs->vmcs->make_current();

    if (mtd.xfer (Mtd::RSP))
        rsp = Vmcs::read (Vmcs::GUEST_RSP);

    if (mtd.xfer (Mtd::RIP_LEN)) {
        rip      = Vmcs::read (Vmcs::GUEST_RIP);
        inst_len = Vmcs::read (Vmcs::EXI_INST_LEN);
    }

    if (mtd.xfer (Mtd::RFLAGS))
        rflags = Vmcs::read (Vmcs::GUEST_RFLAGS);

    if (mtd.xfer (Mtd::DS_ES)) {
        ds.set_vmx (Vmcs::read (Vmcs::GUEST_SEL_DS), Vmcs::read (Vmcs::GUEST_BASE_DS), Vmcs::read (Vmcs::GUEST_LIMIT_DS), Vmcs::read (Vmcs::GUEST_AR_DS));
        es.set_vmx (Vmcs::read (Vmcs::GUEST_SEL_ES), Vmcs::read (Vmcs::GUEST_BASE_ES), Vmcs::read (Vmcs::GUEST_LIMIT_ES), Vmcs::read (Vmcs::GUEST_AR_ES));
    }

    if (mtd.xfer (Mtd::FS_GS)) {
        fs.set_vmx (Vmcs::read (Vmcs::GUEST_SEL_FS), Vmcs::read (Vmcs::GUEST_BASE_FS), Vmcs::read (Vmcs::GUEST_LIMIT_FS), Vmcs::read (Vmcs::GUEST_AR_FS));
        gs.set_vmx (Vmcs::read (Vmcs::GUEST_SEL_GS), Vmcs::read (Vmcs::GUEST_BASE_GS), Vmcs::read (Vmcs::GUEST_LIMIT_GS), Vmcs::read (Vmcs::GUEST_AR_GS));
    }

    if (mtd.xfer (Mtd::CS_SS)) {
        cs.set_vmx (Vmcs::read (Vmcs::GUEST_SEL_CS), Vmcs::read (Vmcs::GUEST_BASE_CS), Vmcs::read (Vmcs::GUEST_LIMIT_CS), Vmcs::read (Vmcs::GUEST_AR_CS));
        ss.set_vmx (Vmcs::read (Vmcs::GUEST_SEL_SS), Vmcs::read (Vmcs::GUEST_BASE_SS), Vmcs::read (Vmcs::GUEST_LIMIT_SS), Vmcs::read (Vmcs::GUEST_AR_SS));
    }

    if (mtd.xfer (Mtd::TR))
        tr.set_vmx (Vmcs::read (Vmcs::GUEST_SEL_TR), Vmcs::read (Vmcs::GUEST_BASE_TR), Vmcs::read (Vmcs::GUEST_LIMIT_TR), Vmcs::read (Vmcs::GUEST_AR_TR));

    if (mtd.xfer (Mtd::LDTR))
        ld.set_vmx (Vmcs::read (Vmcs::GUEST_SEL_LDTR), Vmcs::read (Vmcs::GUEST_BASE_LDTR), Vmcs::read (Vmcs::GUEST_LIMIT_LDTR), Vmcs::read (Vmcs::GUEST_AR_LDTR));

    if (mtd.xfer (Mtd::GDTR))
        gd.set_vmx (0, Vmcs::read (Vmcs::GUEST_BASE_GDTR), Vmcs::read (Vmcs::GUEST_LIMIT_GDTR), 0);

    if (mtd.xfer (Mtd::IDTR))
        id.set_vmx (0, Vmcs::read (Vmcs::GUEST_BASE_IDTR), Vmcs::read (Vmcs::GUEST_LIMIT_IDTR), 0);

    if (mtd.xfer (Mtd::CR)) {
        cr2 = regs->read_cr (2);
        cr3 = regs->read_cr (3);
        cr0 = regs->read_cr (0);
        cr4 = regs->read_cr (4);
    }

    if (mtd.xfer (Mtd::DR))
        dr7 = Vmcs::read (Vmcs::GUEST_DR7);

    if (mtd.xfer (Mtd::SYSENTER)) {
        sysenter_cs  = Vmcs::read (Vmcs::GUEST_SYSENTER_CS);
        sysenter_rsp = Vmcs::read (Vmcs::GUEST_SYSENTER_ESP);
        sysenter_rip = Vmcs::read (Vmcs::GUEST_SYSENTER_EIP);
    }

    if (mtd.xfer (Mtd::QUAL)) {
        qual[0] = Vmcs::read (Vmcs::EXI_QUALIFICATION);
        qual[1] = regs->ept_on ? Vmcs::read (Vmcs::INFO_PHYS_ADDR) : regs->ept_fault;
    }

    if (mtd.xfer (Mtd::INJ)) {
        intr_info  = static_cast<uint32>(Vmcs::read (Vmcs::ENT_INTR_INFO));
        intr_error = static_cast<uint32>(Vmcs::read (Vmcs::ENT_INTR_ERROR));
    }

    if (mtd.xfer (Mtd::STA)) {
        intr_state = static_cast<uint32>(Vmcs::read (Vmcs::GUEST_INTR_STATE));
        actv_state = static_cast<uint32>(Vmcs::read (Vmcs::GUEST_ACTV_STATE));
    }

    if (mtd.xfer (Mtd::TSC)) {
        tsc_lo = static_cast<uint32>(Vmcs::read (Vmcs::TSC_OFFSET));
        tsc_hi = static_cast<uint32>(Vmcs::read (Vmcs::TSC_OFFSET_HI));
    }
}

mword *Utcb::save_vmx (Exc_regs *regs, Mtd mtd)
{
    if (mtd.xfer (Mtd::GPR_ACDB)) {
        regs->eax = rax;
        regs->ecx = rcx;
        regs->edx = rdx;
        regs->ebx = rbx;
    }

    if (mtd.xfer (Mtd::GPR_BSD)) {
        regs->ebp = rbp;
        regs->esi = rsi;
        regs->edi = rdi;
    }

    regs->vmcs->make_current();

    if (mtd.xfer (Mtd::RSP))
        Vmcs::write (Vmcs::GUEST_RSP, rsp);
    if (mtd.xfer (Mtd::RIP_LEN))
        Vmcs::write (Vmcs::GUEST_RIP, rip);
    if (mtd.xfer (Mtd::RFLAGS))
        Vmcs::write (Vmcs::GUEST_RFLAGS, rflags);

    if (mtd.xfer (Mtd::DS_ES)) {
        Vmcs::write (Vmcs::GUEST_SEL_DS,   ds.sel);
        Vmcs::write (Vmcs::GUEST_BASE_DS,  ds.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_DS, ds.limit);
        Vmcs::write (Vmcs::GUEST_AR_DS,   (ds.ar << 4 & 0x1f000) | (ds.ar & 0xff));
        Vmcs::write (Vmcs::GUEST_SEL_ES,   es.sel);
        Vmcs::write (Vmcs::GUEST_BASE_ES,  es.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_ES, es.limit);
        Vmcs::write (Vmcs::GUEST_AR_ES,   (es.ar << 4 & 0x1f000) | (es.ar & 0xff));
    }

    if (mtd.xfer (Mtd::FS_GS)) {
        Vmcs::write (Vmcs::GUEST_SEL_FS,   fs.sel);
        Vmcs::write (Vmcs::GUEST_BASE_FS,  fs.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_FS, fs.limit);
        Vmcs::write (Vmcs::GUEST_AR_FS,   (fs.ar << 4 & 0x1f000) | (fs.ar & 0xff));
        Vmcs::write (Vmcs::GUEST_SEL_GS,   gs.sel);
        Vmcs::write (Vmcs::GUEST_BASE_GS,  gs.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_GS, gs.limit);
        Vmcs::write (Vmcs::GUEST_AR_GS,   (gs.ar << 4 & 0x1f000) | (gs.ar & 0xff));
    }

    if (mtd.xfer (Mtd::CS_SS)) {
        Vmcs::write (Vmcs::GUEST_SEL_CS,   cs.sel);
        Vmcs::write (Vmcs::GUEST_BASE_CS,  cs.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_CS, cs.limit);
        Vmcs::write (Vmcs::GUEST_AR_CS,   (cs.ar << 4 & 0x1f000) | (cs.ar & 0xff));
        Vmcs::write (Vmcs::GUEST_SEL_SS,   ss.sel);
        Vmcs::write (Vmcs::GUEST_BASE_SS,  ss.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_SS, ss.limit);
        Vmcs::write (Vmcs::GUEST_AR_SS,   (ss.ar << 4 & 0x1f000) | (ss.ar & 0xff));
    }

    if (mtd.xfer (Mtd::TR)) {
        Vmcs::write (Vmcs::GUEST_SEL_TR,     tr.sel);
        Vmcs::write (Vmcs::GUEST_BASE_TR,    tr.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_TR,   tr.limit);
        Vmcs::write (Vmcs::GUEST_AR_TR,     (tr.ar << 4 & 0x1f000) | (tr.ar & 0xff));
    }

    if (mtd.xfer (Mtd::LDTR)) {
        Vmcs::write (Vmcs::GUEST_SEL_LDTR,   ld.sel);
        Vmcs::write (Vmcs::GUEST_BASE_LDTR,  ld.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_LDTR, ld.limit);
        Vmcs::write (Vmcs::GUEST_AR_LDTR,   (ld.ar << 4 & 0x1f000) | (ld.ar & 0xff));
    }

    if (mtd.xfer (Mtd::GDTR)) {
        Vmcs::write (Vmcs::GUEST_BASE_GDTR,  gd.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_GDTR, gd.limit);
    }

    if (mtd.xfer (Mtd::IDTR)) {
        Vmcs::write (Vmcs::GUEST_BASE_IDTR,  id.base);
        Vmcs::write (Vmcs::GUEST_LIMIT_IDTR, id.limit);
    }

    if (mtd.xfer (Mtd::CR)) {
        regs->write_cr (2, cr2);
        regs->write_cr (3, cr3);
        regs->write_cr (0, cr0);
        regs->write_cr (4, cr4);
    }

    if (mtd.xfer (Mtd::DR))
        Vmcs::write (Vmcs::GUEST_DR7, dr7);

    if (mtd.xfer (Mtd::SYSENTER)) {
        Vmcs::write (Vmcs::GUEST_SYSENTER_CS,  sysenter_cs);
        Vmcs::write (Vmcs::GUEST_SYSENTER_ESP, sysenter_rsp);
        Vmcs::write (Vmcs::GUEST_SYSENTER_EIP, sysenter_rip);
    }

    if (mtd.xfer (Mtd::CTRL)) {
        regs->set_cpu_ctrl0 (ctrl[0]);
        regs->set_cpu_ctrl1 (ctrl[1]);
    }

    if (mtd.xfer (Mtd::INJ)) {

        uint32 val = static_cast<uint32>(Vmcs::read (Vmcs::CPU_EXEC_CTRL0));

        if (intr_info & 0x1000)
            val |=  Vmcs::CPU_INTR_WINDOW;
        else
            val &= ~Vmcs::CPU_INTR_WINDOW;

        Vmcs::write (Vmcs::CPU_EXEC_CTRL0, val);
        Vmcs::write (Vmcs::ENT_INTR_INFO,  intr_info & ~0x1000);
        Vmcs::write (Vmcs::ENT_INTR_ERROR, intr_error);
    }

    if (mtd.xfer (Mtd::STA)) {
        Vmcs::write (Vmcs::GUEST_INTR_STATE, intr_state);
        Vmcs::write (Vmcs::GUEST_ACTV_STATE, actv_state);
    }

    if (mtd.xfer (Mtd::TSC)) {
        Vmcs::write (Vmcs::TSC_OFFSET,    tsc_lo);
        Vmcs::write (Vmcs::TSC_OFFSET_HI, tsc_hi);
    }

    return item;
}

void Utcb::load_svm (Exc_regs *regs, Mtd mtd)
{
    Vmcb * const vmcb = regs->vmcb;

    mtr = mtd;

    if (mtd.xfer (Mtd::GPR_ACDB)) {
        rax = static_cast<mword>(vmcb->rax);
        rcx = regs->ecx;
        rdx = regs->edx;
        rbx = regs->ebx;
    }

    if (mtd.xfer (Mtd::GPR_BSD)) {
        rbp = regs->ebp;
        rsi = regs->esi;
        rdi = regs->edi;
    }

    if (mtd.xfer (Mtd::RSP))
        rsp = static_cast<mword>(vmcb->rsp);

    if (mtd.xfer (Mtd::RIP_LEN))
        rip = static_cast<mword>(vmcb->rip);

    if (mtd.xfer (Mtd::RFLAGS))
        rflags = static_cast<mword>(vmcb->rflags);

    if (mtd.xfer (Mtd::DS_ES)) {
        ds = vmcb->ds;
        es = vmcb->es;
    }

    if (mtd.xfer (Mtd::FS_GS)) {
        fs = vmcb->fs;
        gs = vmcb->gs;
    }

    if (mtd.xfer (Mtd::CS_SS)) {
        cs = vmcb->cs;
        ss = vmcb->ss;
    }

    if (mtd.xfer (Mtd::TR))
        tr = vmcb->tr;

    if (mtd.xfer (Mtd::LDTR))
        ld = vmcb->ldtr;

    if (mtd.xfer (Mtd::GDTR))
        gd = vmcb->gdtr;

    if (mtd.xfer (Mtd::IDTR))
        id = vmcb->idtr;

    if (mtd.xfer (Mtd::CR)) {
        cr0 = static_cast<mword>(vmcb->cr0);
        cr2 = static_cast<mword>(vmcb->cr2);
        cr3 = static_cast<mword>(vmcb->cr3);
        cr4 = static_cast<mword>(vmcb->cr4);
    }

    if (mtd.xfer (Mtd::DR))
        dr7 = static_cast<mword>(vmcb->dr7);

    if (mtd.xfer (Mtd::SYSENTER)) {
        sysenter_cs  = static_cast<mword>(vmcb->sysenter_cs);
        sysenter_rsp = static_cast<mword>(vmcb->sysenter_esp);
        sysenter_rip = static_cast<mword>(vmcb->sysenter_eip);
    }

    if (mtd.xfer (Mtd::QUAL)) {
        qual[0] = vmcb->exitinfo1;
        qual[1] = vmcb->exitinfo2;
    }

    if (mtd.xfer (Mtd::INJ))
        inj = vmcb->inj_control;

    if (mtd.xfer (Mtd::STA)) {
        intr_state = static_cast<uint32>(vmcb->int_shadow);
        actv_state = 0;
    }

    if (mtd.xfer (Mtd::TSC))
        tsc = vmcb->tsc_offset;
}

mword *Utcb::save_svm (Exc_regs *regs, Mtd mtd)
{
    Vmcb * const vmcb = regs->vmcb;

    if (mtd.xfer (Mtd::GPR_ACDB)) {
        vmcb->rax = rax;
        regs->ecx = rcx;
        regs->edx = rdx;
        regs->ebx = rbx;
    }

    if (mtd.xfer (Mtd::GPR_BSD)) {
        regs->ebp = rbp;
        regs->esi = rsi;
        regs->edi = rdi;
    }

    if (mtd.xfer (Mtd::RSP))
        vmcb->rsp = rsp;

    if (mtd.xfer (Mtd::RIP_LEN))
        vmcb->rip = rip;

    if (mtd.xfer (Mtd::RFLAGS))
        vmcb->rflags = rflags;

    if (mtd.xfer (Mtd::DS_ES)) {
        vmcb->ds = ds;
        vmcb->es = es;
    }

    if (mtd.xfer (Mtd::FS_GS)) {
        vmcb->fs = fs;
        vmcb->gs = gs;
    }

    if (mtd.xfer (Mtd::CS_SS)) {
        vmcb->cs = cs;
        vmcb->ss = ss;
    }

    if (mtd.xfer (Mtd::TR))
        vmcb->tr = tr;

    if (mtd.xfer (Mtd::LDTR))
        vmcb->ldtr = ld;

    if (mtd.xfer (Mtd::GDTR))
        vmcb->gdtr = gd;

    if (mtd.xfer (Mtd::IDTR))
        vmcb->idtr = id;

    if (mtd.xfer (Mtd::CR)) {
        vmcb->cr0 = cr0;
        vmcb->cr2 = cr2;
        vmcb->cr3 = cr3;
        vmcb->cr4 = cr4;
    }

    if (mtd.xfer (Mtd::DR))
        vmcb->dr7 = dr7;

    if (mtd.xfer (Mtd::SYSENTER)) {
        vmcb->sysenter_cs  = sysenter_cs;
        vmcb->sysenter_esp = sysenter_rsp;
        vmcb->sysenter_eip = sysenter_rip;
    }

    if (mtd.xfer (Mtd::CTRL)) {
        vmcb->intercept_cpu[0] = ctrl[0] | Vmcb::force_ctrl0;
        vmcb->intercept_cpu[1] = ctrl[1] | Vmcb::force_ctrl1;
    }

    if (mtd.xfer (Mtd::INJ)) {

        if (intr_info & 0x1000) {
            vmcb->int_control      |=  (1ul << 8 | 1ul << 20);
            vmcb->intercept_cpu[0] |=  Vmcb::CPU_VINTR;
        } else {
            vmcb->int_control      &= ~(1ul << 8 | 1ul << 20);
            vmcb->intercept_cpu[0] &= ~Vmcb::CPU_VINTR;
        }

        vmcb->inj_control = inj;
    }

    if (mtd.xfer (Mtd::STA))
        vmcb->int_shadow = intr_state;

    if (mtd.xfer (Mtd::TSC))
        vmcb->tsc_offset = tsc;

    return item;
}
