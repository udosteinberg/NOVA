/*
 * Register File
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "cpu.hpp"
#include "hip.hpp"
#include "regs.hpp"
#include "svm.hpp"
#include "vmx.hpp"
#include "vpid.hpp"
#include "vtlb.hpp"

template <> mword Exc_regs::get_g_cs_dl<Vmcb>()         const { return static_cast<mword>(vmcb->cs.ar) >> 9 & 0x3; }
template <> mword Exc_regs::get_g_flags<Vmcb>()         const { return static_cast<mword>(vmcb->rflags); }
template <> mword Exc_regs::get_g_efer<Vmcb>()          const { return static_cast<mword>(vmcb->efer); }
template <> mword Exc_regs::get_g_cr0<Vmcb>()           const { return static_cast<mword>(vmcb->cr0); }
template <> mword Exc_regs::get_g_cr2<Vmcb>()           const { return static_cast<mword>(vmcb->cr2); }
template <> mword Exc_regs::get_g_cr3<Vmcb>()           const { return static_cast<mword>(vmcb->cr3); }
template <> mword Exc_regs::get_g_cr4<Vmcb>()           const { return static_cast<mword>(vmcb->cr4); }

template <> mword Exc_regs::get_g_cs_dl<Vmcs>()         const { return Vmcs::read (Vmcs::GUEST_AR_CS) >> 13 & 0x3; }
template <> mword Exc_regs::get_g_flags<Vmcs>()         const { return Vmcs::read (Vmcs::GUEST_RFLAGS); }
template <> mword Exc_regs::get_g_efer<Vmcs>()          const { return Vmcs::read (Vmcs::GUEST_EFER); }
template <> mword Exc_regs::get_g_cr0<Vmcs>()           const { return Vmcs::read (Vmcs::GUEST_CR0); }
template <> mword Exc_regs::get_g_cr2<Vmcs>()           const { return cr2; }
template <> mword Exc_regs::get_g_cr3<Vmcs>()           const { return Vmcs::read (Vmcs::GUEST_CR3); }
template <> mword Exc_regs::get_g_cr4<Vmcs>()           const { return Vmcs::read (Vmcs::GUEST_CR4); }

template <> void Exc_regs::set_g_cr0<Vmcb> (mword v)    const { vmcb->cr0 = v; }
template <> void Exc_regs::set_g_cr2<Vmcb> (mword v)          { vmcb->cr2 = v; }
template <> void Exc_regs::set_g_cr3<Vmcb> (mword v)    const { vmcb->cr3 = v; }
template <> void Exc_regs::set_g_cr4<Vmcb> (mword v)    const { vmcb->cr4 = v; }

template <> void Exc_regs::set_g_cr0<Vmcs> (mword v)    const { Vmcs::write (Vmcs::GUEST_CR0, v); }
template <> void Exc_regs::set_g_cr2<Vmcs> (mword v)          { cr2 = v; }
template <> void Exc_regs::set_g_cr3<Vmcs> (mword v)    const { Vmcs::write (Vmcs::GUEST_CR3, v); }
template <> void Exc_regs::set_g_cr4<Vmcs> (mword v)    const { Vmcs::write (Vmcs::GUEST_CR4, v); }

template <> void Exc_regs::set_e_bmp<Vmcb> (uint32 v)   const { vmcb->intercept_exc = v; }
template <> void Exc_regs::set_s_cr0<Vmcb> (mword v)          { cr0_shadow = v; }
template <> void Exc_regs::set_s_cr4<Vmcb> (mword v)          { cr4_shadow = v; }

template <> void Exc_regs::set_e_bmp<Vmcs> (uint32 v)   const { Vmcs::write (Vmcs::EXC_BITMAP, v); }
template <> void Exc_regs::set_s_cr0<Vmcs> (mword v)          { Vmcs::write (Vmcs::CR0_READ_SHADOW, cr0_shadow = v); }
template <> void Exc_regs::set_s_cr4<Vmcs> (mword v)          { Vmcs::write (Vmcs::CR4_READ_SHADOW, cr4_shadow = v); }

template <> void Exc_regs::tlb_flush<Vmcb>(bool full) const
{
    vtlb->flush (full);

    if (vmcb->asid)
        vmcb->tlb_control = 1;
}

template <> void Exc_regs::tlb_flush<Vmcs>(bool full) const
{
    vtlb->flush (full);

    mword vpid = Vmcs::vpid();

    if (vpid)
        Vpid::flush (full ? Vpid::CONTEXT_GLOBAL : Vpid::CONTEXT_NOGLOBAL, vpid);
}

template <> void Exc_regs::tlb_flush<Vmcs>(mword addr) const
{
    vtlb->flush (addr);

    mword vpid = Vmcs::vpid();

    if (vpid)
        Vpid::flush (Vpid::ADDRESS, vpid, addr);
}

template <typename T>
Exc_regs::Mode Exc_regs::mode() const
{
    if (!(get_cr0<T>() & Cpu::CR0_PE))
        return MODE_REAL;

    if (get_g_flags<T>() & Cpu::EFL_VM)
        return MODE_VM86;

    mword dl = get_g_cs_dl<T>();

    return (get_g_efer<T>() & Cpu::EFER_LMA) && (dl & 1) ? MODE_PROT_64 : dl & 2 ? MODE_PROT_32 : MODE_PROT_16;
}

template <typename T>
mword Exc_regs::linear_address (mword val) const
{
    return mode<T>() == MODE_PROT_64 ? val : val & 0xffffffff;
}

template <typename T>
mword Exc_regs::cr0_set() const
{
    mword set = 0;

    if (!nst_on)
        set |= Cpu::CR0_PG | Cpu::CR0_WP | Cpu::CR0_PE;
    if (!fpu_on)
        set |= Cpu::CR0_TS;

    return T::fix_cr0_set | set;
}

template <typename T>
mword Exc_regs::cr0_msk() const
{
    return T::fix_cr0_clr | cr0_set<T>();
}

template <typename T>
mword Exc_regs::cr4_set() const
{
    mword set = nst_on ? 0 :
#ifdef __i386__
                Cpu::CR4_PSE;
#else
                Cpu::CR4_PSE | Cpu::CR4_PAE;
#endif

    return T::fix_cr4_set | set;
}

template <typename T>
mword Exc_regs::cr4_msk() const
{
    mword clr = nst_on ? 0 :
#ifdef __i386__
                Cpu::CR4_PGE | Cpu::CR4_PAE;
#else
                Cpu::CR4_PGE;
#endif

    return T::fix_cr4_clr | clr | cr4_set<T>();
}

template <typename T>
mword Exc_regs::get_cr0() const
{
    mword msk = cr0_msk<T>();

    return (get_g_cr0<T>() & ~msk) | (cr0_shadow & msk);
}

template <typename T>
mword Exc_regs::get_cr3() const
{
    return nst_on ? get_g_cr3<T>() : cr3_shadow;
}

template <typename T>
mword Exc_regs::get_cr4() const
{
    mword msk = cr4_msk<T>();

    return (get_g_cr4<T>() & ~msk) | (cr4_shadow & msk);
}

template <typename T>
void Exc_regs::set_cr0 (mword v)
{
    set_g_cr0<T> ((v & (~cr0_msk<T>() | Cpu::CR0_PE)) | (cr0_set<T>() & ~Cpu::CR0_PE));
    set_s_cr0<T> (v);
}

template <typename T>
void Exc_regs::set_cr3 (mword v)
{
    if (nst_on)
        set_g_cr3<T> (v);
    else
        cr3_shadow = v;
}

template <typename T>
void Exc_regs::set_cr4 (mword v)
{
    set_g_cr4<T> ((v & ~cr4_msk<T>()) | cr4_set<T>());
    set_s_cr4<T> (v);
}

template <typename T>
void Exc_regs::set_exc() const
{
    unsigned msk = 1UL << Cpu::EXC_AC;

    if (!nst_on)
        msk |= 1UL << Cpu::EXC_PF;
    if (!fpu_on)
        msk |= 1UL << Cpu::EXC_NM;

    set_e_bmp<T> (msk);
}

void Exc_regs::svm_set_cpu_ctrl0 (mword val)
{
    unsigned const msk = !!cr0_msk<Vmcb>() << 0 | !nst_on << 3 | !!cr4_msk<Vmcb>() << 4;

    vmcb->npt_control  = nst_on;
    vmcb->intercept_cr = (msk << 16) | msk;

    if (nst_on)
        vmcb->intercept_cpu[0] = static_cast<uint32>((val & ~Vmcb::CPU_INVLPG) | Vmcb::force_ctrl0);
    else
        vmcb->intercept_cpu[0] = static_cast<uint32>((val |  Vmcb::CPU_INVLPG) | Vmcb::force_ctrl0);
}

void Exc_regs::svm_set_cpu_ctrl1 (mword val)
{
    vmcb->intercept_cpu[1] = static_cast<uint32>(val | Vmcb::force_ctrl1);
}

void Exc_regs::vmx_set_cpu_ctrl0 (mword val)
{
    unsigned const msk = Vmcs::CPU_INVLPG | Vmcs::CPU_CR3_LOAD | Vmcs::CPU_CR3_STORE;

    if (nst_on)
        val &= ~msk;
    else
        val |= msk;

    val |= Vmcs::ctrl_cpu[0].set;
    val &= Vmcs::ctrl_cpu[0].clr;

    Vmcs::write (Vmcs::CPU_EXEC_CTRL0, val);
}

void Exc_regs::vmx_set_cpu_ctrl1 (mword val)
{
    unsigned const msk = Vmcs::CPU_EPT;

    if (nst_on)
        val |= msk;
    else
        val &= ~msk;

    val |= Vmcs::ctrl_cpu[1].set;
    val &= Vmcs::ctrl_cpu[1].clr;

    Vmcs::write (Vmcs::CPU_EXEC_CTRL1, val);
}

template <> void Exc_regs::nst_ctrl<Vmcb>(bool on)
{
    mword cr0 = get_cr0<Vmcb>();
    mword cr3 = get_cr3<Vmcb>();
    mword cr4 = get_cr4<Vmcb>();
    nst_on = Vmcb::has_npt() && on;
    set_cr0<Vmcb> (cr0);
    set_cr3<Vmcb> (cr3);
    set_cr4<Vmcb> (cr4);

    svm_set_cpu_ctrl0 (vmcb->intercept_cpu[0]);
    svm_set_cpu_ctrl1 (vmcb->intercept_cpu[1]);
    set_exc<Vmcb>();

    if (!nst_on)
        set_g_cr3<Vmcb> (Buddy::ptr_to_phys (vtlb));
}

template <> void Exc_regs::nst_ctrl<Vmcs>(bool on)
{
    assert (Vmcs::current == vmcs);

    mword cr0 = get_cr0<Vmcs>();
    mword cr3 = get_cr3<Vmcs>();
    mword cr4 = get_cr4<Vmcs>();
    nst_on = Vmcs::has_ept() && on;
    set_cr0<Vmcs> (cr0);
    set_cr3<Vmcs> (cr3);
    set_cr4<Vmcs> (cr4);

    vmx_set_cpu_ctrl0 (Vmcs::read (Vmcs::CPU_EXEC_CTRL0));
    vmx_set_cpu_ctrl1 (Vmcs::read (Vmcs::CPU_EXEC_CTRL1));
    set_exc<Vmcs>();

    Vmcs::write (Vmcs::CR0_MASK, cr0_msk<Vmcs>());
    Vmcs::write (Vmcs::CR4_MASK, cr4_msk<Vmcs>());

    if (!nst_on)
        set_g_cr3<Vmcs> (Buddy::ptr_to_phys (vtlb));
}

void Exc_regs::fpu_ctrl (bool on)
{
    if (Hip::hip->feature() & Hip::FEAT_VMX) {

        vmcs->make_current();

        mword cr0 = get_cr0<Vmcs>();
        fpu_on = on;
        set_cr0<Vmcs> (cr0);

        set_exc<Vmcs>();

        Vmcs::write (Vmcs::CR0_MASK, cr0_msk<Vmcs>());

    } else {

        mword cr0 = get_cr0<Vmcb>();
        fpu_on = on;
        set_cr0<Vmcb> (cr0);

        set_exc<Vmcb>();

        svm_set_cpu_ctrl0 (vmcb->intercept_cpu[0]);
    }
}

void Exc_regs::svm_update_shadows()
{
    cr0_shadow = get_cr0<Vmcb>();
    cr3_shadow = get_cr3<Vmcb>();
    cr4_shadow = get_cr4<Vmcb>();
}

mword Exc_regs::svm_read_gpr (unsigned reg)
{
    switch (reg) {
        case 0:     return static_cast<mword>(vmcb->rax);
        case 4:     return static_cast<mword>(vmcb->rsp);
        default:    return gpr[sizeof (Sys_regs) / sizeof (mword) - 1 - reg];
    }
}

void Exc_regs::svm_write_gpr (unsigned reg, mword val)
{
    switch (reg) {
        case 0:     vmcb->rax = val; return;
        case 4:     vmcb->rsp = val; return;
        default:    gpr[sizeof (Sys_regs) / sizeof (mword) - 1 - reg] = val; return;
    }
}

mword Exc_regs::vmx_read_gpr (unsigned reg)
{
    if (EXPECT_FALSE (reg == 4))
        return Vmcs::read (Vmcs::GUEST_RSP);
    else
        return gpr[sizeof (Sys_regs) / sizeof (mword) - 1 - reg];
}

void Exc_regs::vmx_write_gpr (unsigned reg, mword val)
{
    if (EXPECT_FALSE (reg == 4))
        Vmcs::write (Vmcs::GUEST_RSP, val);
    else
        gpr[sizeof (Sys_regs) / sizeof (mword) - 1 - reg] = val;
}

template <typename T>
mword Exc_regs::read_cr (unsigned cr) const
{
    switch (cr) {
        case 0: return get_cr0<T>();
        case 2: return get_g_cr2<T>();
        case 3: return get_cr3<T>();
        case 4: return get_cr4<T>();
        default: UNREACHED;
    }
}

template <typename T>
void Exc_regs::write_cr (unsigned cr, mword val)
{
    mword toggled;

    switch (cr) {

        case 2:
            set_g_cr2<T> (val);
            break;

        case 3:
            if (!nst_on)
                tlb_flush<T> (false);

            set_cr3<T> (val);

            break;

        case 0:
            toggled = get_cr0<T>() ^ val;

            if (!nst_on)
                if (toggled & (Cpu::CR0_PG | Cpu::CR0_WP | Cpu::CR0_PE))
                    tlb_flush<T> (true);

            set_cr0<T> (val);

            if (toggled & Cpu::CR0_PG) {

                if (!T::has_urg())
                    nst_ctrl<T> (val & Cpu::CR0_PG);

#ifdef __x86_64__
                mword efer = get_g_efer<T>();
                if ((val & Cpu::CR0_PG) && (efer & Cpu::EFER_LME))
                    write_efer<T> (efer |  Cpu::EFER_LMA);
                else
                    write_efer<T> (efer & ~Cpu::EFER_LMA);
#endif
            }

            break;

        case 4:
            toggled = get_cr4<T>() ^ val;

            if (!nst_on)
                if (toggled & (Cpu::CR4_PGE | Cpu::CR4_PAE | Cpu::CR4_PSE))
                    tlb_flush<T> (true);

            set_cr4<T> (val);

            break;

        default:
            UNREACHED;
    }
}

template <> void Exc_regs::write_efer<Vmcb> (mword val)
{
    vmcb->efer = val;
}

template <> void Exc_regs::write_efer<Vmcs> (mword val)
{
    Vmcs::write (Vmcs::GUEST_EFER, val);

    if (val & Cpu::EFER_LMA)
        Vmcs::write (Vmcs::ENT_CONTROLS, Vmcs::read (Vmcs::ENT_CONTROLS) |  Vmcs::ENT_GUEST_64);
    else
        Vmcs::write (Vmcs::ENT_CONTROLS, Vmcs::read (Vmcs::ENT_CONTROLS) & ~Vmcs::ENT_GUEST_64);
}

template mword Exc_regs::linear_address<Vmcb> (mword) const;
template mword Exc_regs::linear_address<Vmcs> (mword) const;
template mword Exc_regs::read_cr<Vmcb> (unsigned) const;
template mword Exc_regs::read_cr<Vmcs> (unsigned) const;
template void Exc_regs::write_cr<Vmcb> (unsigned, mword);
template void Exc_regs::write_cr<Vmcs> (unsigned, mword);
