/*
 * Execution Context (EC)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

#include "assert.hpp"
#include "cpu.hpp"
#include "ec_arch.hpp"
#include "entry.hpp"
#include "event.hpp"
#include "fpu.hpp"
#include "hip.hpp"
#include "pd.hpp"
#include "rcu.hpp"
#include "space_gst.hpp"
#include "stdio.hpp"
#include "vpid.hpp"

// Constructor: Kernel Thread
Ec_arch::Ec_arch (Refptr<Space_obj> &ref_obj, Refptr<Space_hst> &ref_hst, Refptr<Space_pio> &ref_pio, cpu_t c, cont_t x) : Ec { ref_obj, ref_hst, ref_pio, c, x } {}

// Constructor: HST EC
Ec_arch::Ec_arch (bool t, Fpu *f, Refptr<Space_obj> &ref_obj, Refptr<Space_hst> &ref_hst, Refptr<Space_pio> &ref_pio, cpu_t c, unsigned long e, uintptr_t sp, uintptr_t hva, void *k) : Ec { t, f, ref_obj, ref_hst, ref_pio, k, c, e, t ? send_msg<ret_user_exception> : nullptr }
{
    auto const obj { regs.get_obj() };
    auto const hst { regs.get_hst() };
    auto const pio { regs.get_pio() };

    assert (obj && hst && pio && k);

    trace (TRACE_CREATE, "EC:%p created (OBJ:%p HST:%p PIO:%p CPU:%u UTCB:%p %c)", static_cast<void *>(this), static_cast<void *>(obj), static_cast<void *>(hst), static_cast<void *>(pio), c, static_cast<void *>(k), subtype == Kobject::Subtype::EC_LOCAL ? 'L' : 'G');

    // Make sure we have a PTAB for this CPU in the PD
    hst->init (cpu);

    // FIXME: Allocation failure
    assert (hst->get_ptab (c));

    (t ? exc_regs().rsp : exc_regs().sp()) = sp;
    exc_regs().set_ep (Event::hst_arch + Event::Selector::STARTUP);

    // Map UTCB
    hst->update (hva, Kmem::ptr_to_phys (kpage), 0, Paging::Permissions (Paging::K | Paging::U | Paging::W | Paging::R), Memattr::ram());
}

// Constructor: GST EC (VMX)
Ec_arch::Ec_arch (bool t, Fpu *f, Refptr<Space_obj> &ref_obj, Refptr<Space_hst> &ref_hst, Vmcs *v, cpu_t c, unsigned long e, uintptr_t sp, uint16_t vpid, uintptr_t hva, void *k) : Ec { t, f, ref_obj, ref_hst, v, k, c, e, set_vmm_regs_vmx }
{
    auto const obj { regs.get_obj() };
    auto const hst { regs.get_hst() };

    assert (obj && hst && v && k);

    trace (TRACE_CREATE, "EC:%p created (OBJ:%p HST:%p CPU:%u APIC:%p VMCS:%p %c)", static_cast<void *>(this), static_cast<void *>(obj), static_cast<void *>(hst), c, static_cast<void *>(k), static_cast<void *>(v), subtype == Kobject::Subtype::EC_VCPU_REAL  ? 'R' : 'O');

    // Make sure we have a PTAB for this CPU in the PD
    hst->init (cpu);

    // FIXME: Allocation failure
    assert (hst->get_ptab (c));

    auto const cr3 { Kmem::ptr_to_phys (hst->get_ptab (c)) | (Cpu::feature (Cpu::Feature::PCID) ? hst->get_pcid() : 0) };

    v->init (sp, reinterpret_cast<uintptr_t>(&sys_regs() + 1), cr3, Kmem::ptr_to_phys (kpage), vpid);

    assert (regs.vmcs == Vmcs::current);

    exc_regs().offset_tsc = 0;
    exc_regs().intcpt_cr0 = 0;
    exc_regs().intcpt_cr4 = 0;
    exc_regs().intcpt_exc = 0;
    exc_regs().fpu_on     = false;

    regs.vmx_set_msk_cr0();
    regs.vmx_set_msk_cr4();
    regs.vmx_set_bmp_exc();
    regs.vmx_set_cpu_pri (0);
    regs.vmx_set_cpu_sec (0);

    // Make VMCS inactive on the creator CPU in preparation for migrating it to its target CPU.
    // This ensures the VMCS data is in memory and the VMCS is not active on more than one CPU.
    regs.vmcs->clear();

    exc_regs().set_ep (Event::gst_arch + Event::Selector::STARTUP);

    // Map vAPIC page
    hst->update (hva, Kmem::ptr_to_phys (kpage), 0, Paging::Permissions (Paging::K | Paging::U | Paging::W | Paging::R), Memattr::ram());
}

// Constructor: GST EC (SVM)
Ec_arch::Ec_arch (bool t, Fpu *f, Refptr<Space_obj> &ref_obj, Refptr<Space_hst> &ref_hst, Vmcb *v, cpu_t c, unsigned long e, uintptr_t /*sp*/) : Ec { t, f, ref_obj, ref_hst, v, nullptr, c, e, send_msg<ret_user_vmexit_svm> }
{
    auto const obj { regs.get_obj() };
    auto const hst { regs.get_hst() };

    assert (obj && hst && v);

    trace (TRACE_CREATE, "EC:%p created (OBJ:%p HST:%p CPU:%u VMCB:%p %c)", static_cast<void *>(this), static_cast<void *>(obj), static_cast<void *>(hst), c, static_cast<void *>(v), subtype == Kobject::Subtype::EC_VCPU_REAL  ? 'R' : 'O');

#if 0   // FIXME
    regs.rax = Kmem::ptr_to_phys (regs.vmcb = new Vmcb (pd->Space_pio::walk(), pd->npt.init_root (false)));
#endif

    exc_regs().offset_tsc = 0;
    exc_regs().intcpt_cr0 = 0;
    exc_regs().intcpt_cr4 = 0;
    exc_regs().intcpt_exc = 0;
    exc_regs().fpu_on     = false;

    regs.svm_set_bmp_exc();
    regs.svm_set_cpu_pri (0);
    regs.svm_set_cpu_sec (0);

    exc_regs().set_ep (Event::gst_arch + Event::Selector::STARTUP);
}

// Factory: GST EC
Ec *Ec::create_gst (Status &s, Pd *pd, bool t, bool fpu, cpu_t cpu, unsigned long evt, uintptr_t sp, uintptr_t hva)
{
    auto const has_vmx { Hip::feature (Hip_arch::Feature::VIRT_VMX) };
    auto const has_svm { Hip::feature (Hip_arch::Feature::VIRT_SVM) };

    if (!has_vmx && !has_svm) [[unlikely]] {
        s = Status::BAD_FTR;
        return nullptr;
    }

    // Acquire references
    Refptr<Space_obj> ref_obj { pd->get_obj() };
    Refptr<Space_hst> ref_hst { pd->get_hst() };

    // Failed to acquire references
    if (!ref_obj || !ref_hst) [[unlikely]] {
        s = Status::ABORTED;
        return nullptr;
    }

    auto const f { fpu ? new (pd->fpu_cache) Fpu : nullptr };
    Ec *ec;

    if (has_vmx) {

        auto const v { new Vmcs };
        auto const i { Vpid::allocator.alloc() };
        auto const k { Buddy::alloc (0, Buddy::Fill::BITS0) };

        if ((!fpu || f) && v && i && k && (ec = new (pd->ec_cache) Ec_arch { t, f, ref_obj, ref_hst, v, cpu, evt, sp, i.val(), hva, k })) [[likely]] {
            assert (!ref_obj && !ref_hst);
            return ec;
        }

        if (i)
            Vpid::allocator.free (i.val());

        Buddy::free (k);
        delete v;

    } else if (has_svm) {

        auto const v { new Vmcb };

        if ((!fpu || f) && v && (ec = new (pd->ec_cache) Ec_arch { t, f, ref_obj, ref_hst, v, cpu, evt, sp })) [[likely]] {
            assert (!ref_obj && !ref_hst);
            return ec;
        }

        delete v;
    }

    Fpu::operator delete (f, pd->fpu_cache);

    s = Status::MEM_OBJ;

    return nullptr;
}

void Ec::adjust_offset_ticks (uint64_t t)
{
    if (subtype == Kobject::Subtype::EC_VCPU_OFFS) {
        regs.exc.offset_tsc -= t;
        regs.hazard.set (Hazard::TSC);
    }
}

void Ec::handle_hazard (unsigned h, cont_t func)
{
    if (h & Hazard::RCU)
        Rcu::quiet();

    if (h & (Hazard::ILLEGAL | Hazard::RECALL | Hazard::SLEEP | Hazard::SCHED)) [[unlikely]] {

        Cpu::preemption_point();

        if (Cpu::hazard & Hazard::SLEEP) {      // Reload
            cont = func;
            Cpu::fini();
        }

        if (Cpu::hazard & Hazard::SCHED) {      // Reload
            cont = func;
            Scheduler::schedule();
        }

        if (h & Hazard::ILLEGAL)
            kill ("Illegal execution state");

        if (regs.hazard & Hazard::RECALL) {     // Reload

            regs.hazard.clr (Hazard::RECALL);

            if (func == Ec_arch::ret_user_vmexit_vmx) {
                exc_regs().set_ep (Event::gst_arch + Event::Selector::RECALL);
                send_msg<Ec_arch::ret_user_vmexit_vmx> (this);
            }

            if (func == Ec_arch::ret_user_vmexit_svm) {
                exc_regs().set_ep (Event::gst_arch + Event::Selector::RECALL);
                send_msg<Ec_arch::ret_user_vmexit_svm> (this);
            }

            if (func == Ec_arch::ret_user_hypercall)
                static_cast<Ec_arch *>(this)->redirect_to_iret();

            exc_regs().set_ep (Event::hst_arch + Event::Selector::RECALL);
            send_msg<Ec_arch::ret_user_exception> (this);
        }
    }

    // Point of no return after checking all diversions: this EC will run

    if (h & Hazard::TSC) {

        regs.hazard.clr (Hazard::TSC);

        if (func == Ec_arch::ret_user_vmexit_vmx) {
            regs.vmcs->make_current();
            Vmcs::write<Vmcs::Encoding::TSC_OFFSET>(regs.exc.offset_tsc);
        } else
            regs.vmcb->tsc_offset = regs.exc.offset_tsc;
    }

    if (h & Hazard::FPU) [[unlikely]]
        Cpu::hazard & Hazard::FPU ? Fpu::disable() : Fpu::enable();
}

void Ec_arch::ret_user_hypercall (Ec *const self)
{
    auto &r { self->regs };

    auto const h { (Cpu::hazard ^ r.hazard) & (Hazard::ILLEGAL | Hazard::RECALL | Hazard::FPU | Hazard::RCU | Hazard::SLEEP | Hazard::SCHED) };
    if (h) [[unlikely]]
        self->handle_hazard (h, ret_user_hypercall);

    trace (TRACE_CONT, "EC:%p %s to CS:%#x IP:%#lx", static_cast<void *>(self), __func__, SEL_USER_CODE, r.exc.ip());

    Cet::sss_deactivate();

    asm volatile ("lea %0, %%rsp;" EXPAND (LOAD_GPR) "mov %%r11, %%rsp; mov %1, %%r11; sysretq" : : "m" (r.exc), "i" (RFL_IF | RFL_1) : "memory");

    UNREACHED;
}

void Ec_arch::ret_user_exception (Ec *const self)
{
    auto const &r { self->regs };

    auto const h { (Cpu::hazard ^ r.hazard) & (Hazard::ILLEGAL | Hazard::RECALL | Hazard::FPU | Hazard::RCU | Hazard::SLEEP | Hazard::SCHED) };
    if (h) [[unlikely]]
        self->handle_hazard (h, ret_user_exception);

    trace (TRACE_CONT, "EC:%p %s to CS:%#lx IP:%#lx", static_cast<void *>(self), __func__, r.exc.cs, r.exc.rip);

    Cet::sss_unwind();

    asm volatile ("lea %0, %%rsp;" EXPAND (LOAD_GPR IRET) : : "m" (r.exc) : "memory");

    UNREACHED;
}

void Ec_arch::ret_user_vmexit_vmx (Ec *const self)
{
    auto const &r { self->regs };

    auto const h { (Cpu::hazard ^ r.hazard) & (Hazard::ILLEGAL | Hazard::RECALL | Hazard::TSC | Hazard::RCU | Hazard::SLEEP | Hazard::SCHED) };
    if (h) [[unlikely]]
        self->handle_hazard (h, ret_user_vmexit_vmx);

    r.vmcs->make_current();

    trace (TRACE_CONT, "EC:%p %s to CS:%#x IP:%#lx", static_cast<void *>(self), __func__, Vmcs::read<Vmcs::Encoding::GUEST_SEL_CS>(), Vmcs::read<Vmcs::Encoding::GUEST_RIP>());

    auto const gst { r.get_gst() };

    if (gst->gtlb.tst (Cpu::id)) [[unlikely]] {
        gst->gtlb.clr (Cpu::id);
        gst->invalidate();
    }

    if (Cr::get_cr2() != r.cr2) [[unlikely]]
        Cr::set_cr2 (r.cr2);

    r.gst_sys.make_current (Cpu::hst_sys);              // Restore SYS guest state
    r.gst_tsc.make_current (Cpu::hst_tsc);              // Restore TSC guest state
    r.gst_xsv.make_current (Fpu::hst_xsv);              // Restore XSV guest state
    r.gst_sgx.make_current();                           // Restore SGX guest state

    asm volatile ("lea %0, %%rsp;"
                  EXPAND (LOAD_GPR)
                  "vmresume;"
                  "vmlaunch;"
                  "lea %1, %%rsp;"
                  "jmp vmx_failure;"
                  : : "m" (r.exc), "m" (DSTK_TOP) : "memory");

    UNREACHED;
}

void Ec_arch::ret_user_vmexit_svm (Ec *const self)
{
    auto const &r { self->regs };

    auto const h { (Cpu::hazard ^ r.hazard) & (Hazard::ILLEGAL | Hazard::RECALL | Hazard::TSC | Hazard::RCU | Hazard::SLEEP | Hazard::SCHED) };
    if (h) [[unlikely]]
        self->handle_hazard (h, ret_user_vmexit_svm);

    auto const gst { r.get_gst() };

    if (gst->gtlb.tst (Cpu::id)) [[unlikely]] {
        gst->gtlb.clr (Cpu::id);
        r.vmcb->tlb_control = 1;
    }

    r.gst_tsc.make_current (Cpu::hst_tsc);              // Restore TSC guest state
    r.gst_xsv.make_current (Fpu::hst_xsv);              // Restore XSV guest state

    asm volatile ("lea %0, %%rsp;"
                  EXPAND (LOAD_GPR)
                  "clgi;"
                  "sti;"
                  "vmload;"
                  "vmrun;"
                  "vmsave;"
                  EXPAND (SAVE_GPR)
                  "mov %1, %%rax;"
                  "lea %2, %%rsp;"
                  "vmload;"
                  "cli;"
                  "stgi;"
                  "jmp svm_handler;"
                  : : "m" (r.exc), "m" (Vmcb::root), "m" (DSTK_TOP) : "memory");

    UNREACHED;
}

void Ec_arch::set_vmm_regs_vmx (Ec *const self)
{
    assert (self->is_vcpu());
    assert (self->cpu == Cpu::id);

    auto &c { self->cpu_regs() };

    c.gst_sgx = Sgx::initial;

    auto &s { c.exc.sys };

    s.rax = Vmcs::cpu_pri_clr;
    s.rcx = Vmcs::cpu_sec_clr;
    s.rdx = Vmcs::cpu_ter_clr;

    s.rdi = Msr::read (Msr::Reg64::IA32_FEATURE_CONTROL);   // Use of VMX guarantees its existence

    s.r8  = Cpu::feature (Cpu::Feature::ARCH_CAPABILITIES) ? Msr::read (Msr::Reg64::IA32_ARCH_CAPABILITIES) : 0;
    s.r9  = Cpu::feature (Cpu::Feature::CORE_CAPABILITIES) ? Msr::read (Msr::Reg64::IA32_CORE_CAPABILITIES) : 0;
    s.r10 = Cpu::feature (Cpu::Feature::SRBDS_CTRL) || s.r8 & BIT (18) ? Msr::read (Msr::Reg64::IA32_MCU_OPT_CTRL) : 0;

    send_msg<ret_user_vmexit_vmx> (self);
}
