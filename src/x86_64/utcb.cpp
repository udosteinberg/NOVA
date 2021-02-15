/*
 * User Thread Control Block (UTCB)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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
#include "mtd.hpp"
#include "regs.hpp"
#include "svm.hpp"
#include "vmx.hpp"

bool Utcb::load_exc (Cpu_regs const &c)
{
    auto const &e { c.exc };
    auto const &s { e.sys };

    mword m = c.mtd;

    if (m & Mtd::GPR_0_3) {
        rax = s.rax;
        rcx = s.rcx;
        rdx = s.rdx;
        rbx = s.rbx;
    }

    if (m & Mtd::GPR_4_7) {
        rsp = e.rsp;
        rbp = s.rbp;
        rsi = s.rsi;
        rdi = s.rdi;
    }

    if (m & Mtd::GPR_8_15) {
        r8  = s.r8;
        r9  = s.r9;
        r10 = s.r10;
        r11 = s.r11;
        r12 = s.r12;
        r13 = s.r13;
        r14 = s.r14;
        r15 = s.r15;
    }

    if (m & Mtd::RIP_LEN)
        rip = e.rip;

    if (m & Mtd::RFLAGS)
        rflags = e.rfl;

    if (m & Mtd::QUAL) {
        qual[0] = e.err;
        qual[1] = c.cr2;
    }

    barrier();
    mtd = m;
    items = sizeof (Utcb_data) / sizeof (mword);

    return m & Mtd::FPU;
}

bool Utcb::save_exc (Cpu_regs &c) const
{
    auto &e { c.exc };
    auto &s { e.sys };

    if (mtd & Mtd::GPR_0_3) {
        s.rax = rax;
        s.rcx = rcx;
        s.rdx = rdx;
        s.rbx = rbx;
    }

    if (mtd & Mtd::GPR_4_7) {
        e.rsp = rsp;
        s.rbp = rbp;
        s.rsi = rsi;
        s.rdi = rdi;
    }

    if (mtd & Mtd::GPR_8_15) {
        s.r8  = r8;
        s.r9  = r9;
        s.r10 = r10;
        s.r11 = r11;
        s.r12 = r12;
        s.r13 = r13;
        s.r14 = r14;
        s.r15 = r15;
    }

    if (mtd & Mtd::RIP_LEN)
        e.rip = rip;

    if (mtd & Mtd::RFLAGS)
        e.rfl = (rflags & ~(RFL_VIP | RFL_VIF | RFL_VM | RFL_RF | RFL_IOPL)) | RFL_IF;

    return mtd & Mtd::FPU;
}

bool Utcb::load_vmx (Cpu_regs const &c)
{
    auto const &s { c.exc.sys };

    c.vmcs->make_current();

    mword m = c.mtd;

    if (m & Mtd::GPR_0_3) {
        rax = s.rax;
        rcx = s.rcx;
        rdx = s.rdx;
        rbx = s.rbx;
    }

    if (m & Mtd::GPR_4_7) {
        rsp = Vmcs::read<mword> (Vmcs::GUEST_RSP);
        rbp = s.rbp;
        rsi = s.rsi;
        rdi = s.rdi;
    }

    if (m & Mtd::GPR_8_15) {
        r8  = s.r8;
        r9  = s.r9;
        r10 = s.r10;
        r11 = s.r11;
        r12 = s.r12;
        r13 = s.r13;
        r14 = s.r14;
        r15 = s.r15;
    }

    if (m & Mtd::RIP_LEN) {
        rip      = Vmcs::read<mword>  (Vmcs::GUEST_RIP);
        inst_len = Vmcs::read<uint32> (Vmcs::EXI_INST_LEN);
    }

    if (m & Mtd::RFLAGS)
        rflags = Vmcs::read<mword> (Vmcs::GUEST_RFLAGS);

    if (m & Mtd::DS_ES) {
        ds.set_vmx (Vmcs::read<uint16> (Vmcs::GUEST_SEL_DS), Vmcs::read<mword> (Vmcs::GUEST_BASE_DS), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_DS), Vmcs::read<uint32> (Vmcs::GUEST_AR_DS));
        es.set_vmx (Vmcs::read<uint16> (Vmcs::GUEST_SEL_ES), Vmcs::read<mword> (Vmcs::GUEST_BASE_ES), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_ES), Vmcs::read<uint32> (Vmcs::GUEST_AR_ES));
    }

    if (m & Mtd::FS_GS) {
        fs.set_vmx (Vmcs::read<uint16> (Vmcs::GUEST_SEL_FS), Vmcs::read<mword> (Vmcs::GUEST_BASE_FS), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_FS), Vmcs::read<uint32> (Vmcs::GUEST_AR_FS));
        gs.set_vmx (Vmcs::read<uint16> (Vmcs::GUEST_SEL_GS), Vmcs::read<mword> (Vmcs::GUEST_BASE_GS), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_GS), Vmcs::read<uint32> (Vmcs::GUEST_AR_GS));
    }

    if (m & Mtd::CS_SS) {
        cs.set_vmx (Vmcs::read<uint16> (Vmcs::GUEST_SEL_CS), Vmcs::read<mword> (Vmcs::GUEST_BASE_CS), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_CS), Vmcs::read<uint32> (Vmcs::GUEST_AR_CS));
        ss.set_vmx (Vmcs::read<uint16> (Vmcs::GUEST_SEL_SS), Vmcs::read<mword> (Vmcs::GUEST_BASE_SS), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_SS), Vmcs::read<uint32> (Vmcs::GUEST_AR_SS));
    }

    if (m & Mtd::TR)
        tr.set_vmx (Vmcs::read<uint16> (Vmcs::GUEST_SEL_TR), Vmcs::read<mword> (Vmcs::GUEST_BASE_TR), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_TR), Vmcs::read<uint32> (Vmcs::GUEST_AR_TR));

    if (m & Mtd::LDTR)
        ld.set_vmx (Vmcs::read<uint16> (Vmcs::GUEST_SEL_LDTR), Vmcs::read<mword> (Vmcs::GUEST_BASE_LDTR), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_LDTR), Vmcs::read<uint32> (Vmcs::GUEST_AR_LDTR));

    if (m & Mtd::GDTR)
        gd.set_vmx (0, Vmcs::read<mword> (Vmcs::GUEST_BASE_GDTR), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_GDTR), 0);

    if (m & Mtd::IDTR)
        id.set_vmx (0, Vmcs::read<mword> (Vmcs::GUEST_BASE_IDTR), Vmcs::read<uint32> (Vmcs::GUEST_LIMIT_IDTR), 0);

    if (m & Mtd::CR) {
        cr0 = c.vmx_get_gst_cr0();
        cr4 = c.vmx_get_gst_cr4();
        cr2 = c.cr2;
        cr3 = Vmcs::read<mword> (Vmcs::Encoding::GUEST_CR3);
        pdpte[0] = Vmcs::read<uint64> (Vmcs::GUEST_PDPTE0);
        pdpte[1] = Vmcs::read<uint64> (Vmcs::GUEST_PDPTE1);
        pdpte[2] = Vmcs::read<uint64> (Vmcs::GUEST_PDPTE2);
        pdpte[3] = Vmcs::read<uint64> (Vmcs::GUEST_PDPTE3);
    }

    if (m & Mtd::DR)
        dr7 = Vmcs::read<mword> (Vmcs::GUEST_DR7);

    if (m & Mtd::SYSENTER) {
        sysenter_cs  = Vmcs::read<uint32> (Vmcs::GUEST_SYSENTER_CS);
        sysenter_rsp = Vmcs::read<mword>  (Vmcs::GUEST_SYSENTER_ESP);
        sysenter_rip = Vmcs::read<mword>  (Vmcs::GUEST_SYSENTER_EIP);
    }

    if (m & Mtd::QUAL) {
        qual[0] = Vmcs::read<mword>  (Vmcs::EXI_QUALIFICATION);
        qual[1] = Vmcs::read<uint64> (Vmcs::INFO_PHYS_ADDR);
    }

    if (m & Mtd::INJ) {
        if (c.exc.ep() == 33 || c.exc.ep() == NUM_VMI - 1) {
            intr_info  = Vmcs::read<uint32> (Vmcs::ENT_INTR_INFO);
            intr_error = Vmcs::read<uint32> (Vmcs::ENT_INTR_ERROR);
        } else {
            intr_info  = Vmcs::read<uint32> (Vmcs::IDT_VECT_INFO);
            intr_error = Vmcs::read<uint32> (Vmcs::IDT_VECT_ERROR);
        }
    }

    if (m & Mtd::STA) {
        intr_state = Vmcs::read<uint32> (Vmcs::GUEST_INTR_STATE);
        actv_state = Vmcs::read<uint32> (Vmcs::GUEST_ACTV_STATE);
    }

    if (m & Mtd::TSC) {
        tsc_val = rdtsc();
        tsc_off = c.tsc_offset;
    }

    if (m & Mtd::EFER)
        efer = Vmcs::read<uint64> (Vmcs::GUEST_EFER);

    barrier();
    mtd = m;
    items = sizeof (Utcb_data) / sizeof (mword);

    return m & Mtd::FPU;
}

bool Utcb::save_vmx (Cpu_regs &c) const
{
    auto &s { c.exc.sys };

    c.vmcs->make_current();

    if (mtd & Mtd::GPR_0_3) {
        s.rax = rax;
        s.rcx = rcx;
        s.rdx = rdx;
        s.rbx = rbx;
    }

    if (mtd & Mtd::GPR_4_7) {
        Vmcs::write (Vmcs::GUEST_RSP, rsp);
        s.rbp = rbp;
        s.rsi = rsi;
        s.rdi = rdi;
    }

    if (mtd & Mtd::GPR_8_15) {
        s.r8  = r8;
        s.r9  = r9;
        s.r10 = r10;
        s.r11 = r11;
        s.r12 = r12;
        s.r13 = r13;
        s.r14 = r14;
        s.r15 = r15;
    }

    if (mtd & Mtd::RIP_LEN) {
        Vmcs::write (Vmcs::GUEST_RIP, rip);
        Vmcs::write (Vmcs::ENT_INST_LEN, static_cast<uint32> (inst_len));
    }

    if (mtd & Mtd::RFLAGS)
        Vmcs::write (Vmcs::GUEST_RFLAGS, rflags);

    if (mtd & Mtd::DS_ES) {
        Vmcs::write (Vmcs::GUEST_SEL_DS,   ds.sel);
        Vmcs::write (Vmcs::GUEST_BASE_DS,  static_cast<mword>(ds.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_DS, ds.limit);
        Vmcs::write (Vmcs::GUEST_AR_DS,   (ds.ar << 4 & 0x1f000) | (ds.ar & 0xff));
        Vmcs::write (Vmcs::GUEST_SEL_ES,   es.sel);
        Vmcs::write (Vmcs::GUEST_BASE_ES,  static_cast<mword>(es.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_ES, es.limit);
        Vmcs::write (Vmcs::GUEST_AR_ES,   (es.ar << 4 & 0x1f000) | (es.ar & 0xff));
    }

    if (mtd & Mtd::FS_GS) {
        Vmcs::write (Vmcs::GUEST_SEL_FS,   fs.sel);
        Vmcs::write (Vmcs::GUEST_BASE_FS,  static_cast<mword>(fs.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_FS, fs.limit);
        Vmcs::write (Vmcs::GUEST_AR_FS,   (fs.ar << 4 & 0x1f000) | (fs.ar & 0xff));
        Vmcs::write (Vmcs::GUEST_SEL_GS,   gs.sel);
        Vmcs::write (Vmcs::GUEST_BASE_GS,  static_cast<mword>(gs.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_GS, gs.limit);
        Vmcs::write (Vmcs::GUEST_AR_GS,   (gs.ar << 4 & 0x1f000) | (gs.ar & 0xff));
    }

    if (mtd & Mtd::CS_SS) {
        Vmcs::write (Vmcs::GUEST_SEL_CS,   cs.sel);
        Vmcs::write (Vmcs::GUEST_BASE_CS,  static_cast<mword>(cs.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_CS, cs.limit);
        Vmcs::write (Vmcs::GUEST_AR_CS,   (cs.ar << 4 & 0x1f000) | (cs.ar & 0xff));
        Vmcs::write (Vmcs::GUEST_SEL_SS,   ss.sel);
        Vmcs::write (Vmcs::GUEST_BASE_SS,  static_cast<mword>(ss.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_SS, ss.limit);
        Vmcs::write (Vmcs::GUEST_AR_SS,   (ss.ar << 4 & 0x1f000) | (ss.ar & 0xff));
    }

    if (mtd & Mtd::TR) {
        Vmcs::write (Vmcs::GUEST_SEL_TR,     tr.sel);
        Vmcs::write (Vmcs::GUEST_BASE_TR,    static_cast<mword>(tr.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_TR,   tr.limit);
        Vmcs::write (Vmcs::GUEST_AR_TR,     (tr.ar << 4 & 0x1f000) | (tr.ar & 0xff));
    }

    if (mtd & Mtd::LDTR) {
        Vmcs::write (Vmcs::GUEST_SEL_LDTR,   ld.sel);
        Vmcs::write (Vmcs::GUEST_BASE_LDTR,  static_cast<mword>(ld.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_LDTR, ld.limit);
        Vmcs::write (Vmcs::GUEST_AR_LDTR,   (ld.ar << 4 & 0x1f000) | (ld.ar & 0xff));
    }

    if (mtd & Mtd::GDTR) {
        Vmcs::write (Vmcs::GUEST_BASE_GDTR,  static_cast<mword>(gd.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_GDTR, gd.limit);
    }

    if (mtd & Mtd::IDTR) {
        Vmcs::write (Vmcs::GUEST_BASE_IDTR,  static_cast<mword>(id.base));
        Vmcs::write (Vmcs::GUEST_LIMIT_IDTR, id.limit);
    }

    if (mtd & Mtd::CR) {
        c.vmx_set_gst_cr0 (cr0);
        c.vmx_set_gst_cr4 (cr4);
        c.cr2 = cr2;
        Vmcs::write (Vmcs::Encoding::GUEST_CR3, cr3);
        Vmcs::write (Vmcs::GUEST_PDPTE0, pdpte[0]);
        Vmcs::write (Vmcs::GUEST_PDPTE1, pdpte[1]);
        Vmcs::write (Vmcs::GUEST_PDPTE2, pdpte[2]);
        Vmcs::write (Vmcs::GUEST_PDPTE3, pdpte[3]);
    }

    if (mtd & Mtd::DR)
        Vmcs::write (Vmcs::GUEST_DR7, dr7);

    if (mtd & Mtd::SYSENTER) {
        Vmcs::write (Vmcs::GUEST_SYSENTER_CS,  sysenter_cs);
        Vmcs::write (Vmcs::GUEST_SYSENTER_ESP, sysenter_rsp);
        Vmcs::write (Vmcs::GUEST_SYSENTER_EIP, sysenter_rip);
    }

    if (mtd & Mtd::CTRL) {
        c.vmx_set_cpu_pri (ctrl[0]);
        c.vmx_set_cpu_sec (ctrl[1]);
    }

    if (mtd & Mtd::INJ) {

        uint32 val = Vmcs::read<uint32> (Vmcs::CPU_EXEC_CTRL0);

        if (intr_info & 0x1000)
            val |=  Vmcs::CPU_INTR_WINDOW;
        else
            val &= ~Vmcs::CPU_INTR_WINDOW;

        if (intr_info & 0x2000)
            val |=  Vmcs::CPU_NMI_WINDOW;
        else
            val &= ~Vmcs::CPU_NMI_WINDOW;

        c.vmx_set_cpu_pri (val);

        Vmcs::write (Vmcs::ENT_INTR_INFO,  intr_info & ~0x3000);
        Vmcs::write (Vmcs::ENT_INTR_ERROR, intr_error);
    }

    if (mtd & Mtd::STA) {
        Vmcs::write (Vmcs::GUEST_INTR_STATE, intr_state);
        Vmcs::write (Vmcs::GUEST_ACTV_STATE, actv_state);
    }

    if (mtd & Mtd::EFER) {
    
        Vmcs::write (Vmcs::GUEST_EFER, efer);

        auto ent { Vmcs::read<uint32> (Vmcs::ENT_CONTROLS) };

        if (efer & EFER_LMA)
            ent |= Vmcs::ENT_GUEST_64;
        else
            ent &= ~Vmcs::ENT_GUEST_64;

        Vmcs::write (Vmcs::ENT_CONTROLS, ent);
    }

    return mtd & Mtd::FPU;
}

bool Utcb::load_svm (Cpu_regs const &c)
{
    auto const &s { c.exc.sys };
    auto const  v { c.vmcb };

    mword m = c.mtd;

    if (m & Mtd::GPR_0_3) {
        rax = v->rax;
        rcx = s.rcx;
        rdx = s.rdx;
        rbx = s.rbx;
    }

    if (m & Mtd::GPR_4_7) {
        rsp = v->rsp;
        rbp = s.rbp;
        rsi = s.rsi;
        rdi = s.rdi;
    }

    if (m & Mtd::GPR_8_15) {
        r8  = s.r8;
        r9  = s.r9;
        r10 = s.r10;
        r11 = s.r11;
        r12 = s.r12;
        r13 = s.r13;
        r14 = s.r14;
        r15 = s.r15;
    }

    if (m & Mtd::RIP_LEN)
        rip = v->rip;

    if (m & Mtd::RFLAGS)
        rflags = v->rflags;

    if (m & Mtd::DS_ES) {
        ds = v->ds;
        es = v->es;
    }

    if (m & Mtd::FS_GS) {
        fs = v->fs;
        gs = v->gs;
    }

    if (m & Mtd::CS_SS) {
        cs = v->cs;
        ss = v->ss;
    }

    if (m & Mtd::TR)
        tr = v->tr;

    if (m & Mtd::LDTR)
        ld = v->ldtr;

    if (m & Mtd::GDTR)
        gd = v->gdtr;

    if (m & Mtd::IDTR)
        id = v->idtr;

    if (m & Mtd::CR) {
        cr0 = v->cr0;
        cr2 = v->cr2;
        cr3 = v->cr3;
        cr4 = v->cr4;
    }

    if (m & Mtd::DR)
        dr7 = v->dr7;

    if (m & Mtd::SYSENTER) {
        sysenter_cs  = v->sysenter_cs;
        sysenter_rsp = v->sysenter_esp;
        sysenter_rip = v->sysenter_eip;
    }

    if (m & Mtd::QUAL) {
        qual[0] = v->exitinfo1;
        qual[1] = v->exitinfo2;
    }

    if (m & Mtd::INJ) {
        if (c.exc.ep() == NUM_VMI - 3 || c.exc.ep() == NUM_VMI - 1)
            inj = v->inj_control;
        else
            inj = v->exitintinfo;
    }

    if (m & Mtd::STA) {
        intr_state = static_cast<uint32>(v->int_shadow);
        actv_state = 0;
    }

    if (m & Mtd::TSC) {
        tsc_val = rdtsc();
        tsc_off = c.tsc_offset;
    }

    if (m & Mtd::EFER)
        efer = v->efer;

    barrier();
    mtd = m;
    items = sizeof (Utcb_data) / sizeof (mword);

    return m & Mtd::FPU;
}

bool Utcb::save_svm (Cpu_regs &c) const
{
    auto &s { c.exc.sys };
    auto  v { c.vmcb };

    if (mtd & Mtd::GPR_0_3) {
        v->rax = rax;
        s.rcx = rcx;
        s.rdx = rdx;
        s.rbx = rbx;
    }

    if (mtd & Mtd::GPR_4_7) {
        v->rsp = rsp;
        s.rbp = rbp;
        s.rsi = rsi;
        s.rdi = rdi;
    }

    if (mtd & Mtd::GPR_8_15) {
        s.r8  = r8;
        s.r9  = r9;
        s.r10 = r10;
        s.r11 = r11;
        s.r12 = r12;
        s.r13 = r13;
        s.r14 = r14;
        s.r15 = r15;
    }

    if (mtd & Mtd::RIP_LEN)
        v->rip = rip;

    if (mtd & Mtd::RFLAGS)
        v->rflags = rflags;

    if (mtd & Mtd::DS_ES) {
        v->ds = ds;
        v->es = es;
    }

    if (mtd & Mtd::FS_GS) {
        v->fs = fs;
        v->gs = gs;
    }

    if (mtd & Mtd::CS_SS) {
        v->cs = cs;
        v->ss = ss;
    }

    if (mtd & Mtd::TR)
        v->tr = tr;

    if (mtd & Mtd::LDTR)
        v->ldtr = ld;

    if (mtd & Mtd::GDTR)
        v->gdtr = gd;

    if (mtd & Mtd::IDTR)
        v->idtr = id;

    if (mtd & Mtd::CR) {
        v->cr0 = cr0;
        v->cr2 = cr2;
        v->cr3 = cr3;
        v->cr4 = cr4;
    }

    if (mtd & Mtd::DR)
        v->dr7 = dr7;

    if (mtd & Mtd::SYSENTER) {
        v->sysenter_cs  = sysenter_cs;
        v->sysenter_esp = sysenter_rsp;
        v->sysenter_eip = sysenter_rip;
    }

    if (mtd & Mtd::CTRL) {
        c.svm_set_cpu_pri (ctrl[0]);
        c.svm_set_cpu_sec (ctrl[1]);
    }

    if (mtd & Mtd::INJ) {

        if (intr_info & 0x1000) {
            v->int_control      |=  (1ul << 8 | 1ul << 20);
            v->intercept_cpu[0] |=  Vmcb::CPU_VINTR;
        } else {
            v->int_control      &= ~(1ul << 8 | 1ul << 20);
            v->intercept_cpu[0] &= ~Vmcb::CPU_VINTR;
        }

        v->inj_control = inj & ~0x3000;
    }

    if (mtd & Mtd::STA)
        v->int_shadow = intr_state;

    if (mtd & Mtd::EFER)
        v->efer = efer;

    return mtd & Mtd::FPU;
}
