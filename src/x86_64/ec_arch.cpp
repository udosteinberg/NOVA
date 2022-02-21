/*
 * Execution Context (EC)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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
#include "multiboot.hpp"
#include "pd.hpp"
#include "rcu.hpp"
#include "sc.hpp"
#include "space_gst.hpp"
#include "stdio.hpp"
#include "timer.hpp"
#include "vpid.hpp"

// Constructor: Kernel Thread
Ec_arch::Ec_arch (unsigned c, cont_t x) : Ec (&Space_hst::nova, c, x) {}

// Constructor: User Thread
Ec_arch::Ec_arch (Space_obj *obj, Space_hst *hst, Space_pio *pio, Fpu *f, Utcb *u, unsigned c, unsigned long e, bool t, uintptr_t a, uintptr_t s) : Ec (obj, hst, pio, f, u, c, e, t, t ? send_msg<ret_user_exception> : nullptr)
{
    assert (obj && hst && pio && u);

    trace (TRACE_CREATE, "EC:%p created (OBJ:%p HST:%p PIO:%p CPU:%u UTCB:%p %c)", static_cast<void *>(this), static_cast<void *>(obj), static_cast<void *>(hst), static_cast<void *>(pio), c, static_cast<void *>(u), subtype == Kobject::Subtype::EC_LOCAL ? 'L' : 'G');

    (t ? exc_regs().rsp : exc_regs().sp()) = s;
    exc_regs().set_ep (Event::hst_arch + Event::Selector::STARTUP);

    // Make sure we have a PTAB for this CPU in the PD
    hst->init (cpu);

    // FIXME: Allocation failure
    assert (hst->get_ptab (c));

    // Map UTCB
    hst->update (a, Kmem::ptr_to_phys (utcb), 0, Paging::Permissions (Paging::K | Paging::U | Paging::W | Paging::R), Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);
}

// Constructor: Virtual CPU (VMX)
template <>
Ec_arch::Ec_arch (Space_obj *obj, Space_hst *hst, Fpu *f, Vmcs *v, unsigned c, unsigned long e, bool t) : Ec (obj, hst, f, v, c, e, t, send_msg<ret_user_vmexit_vmx>)
{
    assert (obj && hst && v);

    trace (TRACE_CREATE, "EC:%p created (OBJ:%p HST:%p CPU:%u VMCS:%p %c)", static_cast<void *>(this), static_cast<void *>(obj), static_cast<void *>(hst), c, static_cast<void *>(v), subtype == Kobject::Subtype::EC_VCPU_REAL  ? 'R' : 'O');

    // Make sure we have a PTAB for this CPU in the PD
    hst->init (cpu);

    // FIXME: Allocation failure
    assert (hst->get_ptab (c));

    auto const cr3 { Kmem::ptr_to_phys (hst->get_ptab (c)) | (Cpu::feature (Cpu::Feature::PCID) ? hst->get_pcid() : 0) };
    v->init (reinterpret_cast<uintptr_t>(&sys_regs() + 1), cr3, Vpid::alloc (cpu));

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
}

// Constructor: Virtual CPU (SVM)
template <>
Ec_arch::Ec_arch (Space_obj *obj, Space_hst *hst, Fpu *f, Vmcb *v, unsigned c, unsigned long e, bool t) : Ec (obj, hst, f, v, c, e, t, send_msg<ret_user_vmexit_svm>)
{
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

// Factory: Virtual CPU
Ec *Ec::create (Status &s, Pd *pd, bool fpu, unsigned c, unsigned long e, bool t)
{
    auto const has_vmx { Hip::feature (Hip_arch::Feature::VMX) };
    auto const has_svm { Hip::feature (Hip_arch::Feature::SVM) };

    if (EXPECT_FALSE (!has_vmx && !has_svm)) {
        s = Status::BAD_FTR;
        return nullptr;
    }

    auto const obj { pd->get_obj() };
    auto const hst { pd->get_hst() };

    if (EXPECT_FALSE (!obj || !hst)) {
        s = Status::ABORTED;
        return nullptr;
    }

    // FIXME: Refcount updates

    auto const f { fpu ? new (pd->fpu_cache) Fpu : nullptr };
    Ec *ec;

    if (has_vmx) {
        auto const v { new Vmcs };
        if (EXPECT_TRUE ((!fpu || f) && v && (ec = new (cache) Ec_arch (obj, hst, f, v, c, e, t))))
            return ec;
        delete v;
    }

    if (has_svm) {
        auto const v { new Vmcb };
        if (EXPECT_TRUE ((!fpu || f) && v && (ec = new (cache) Ec_arch (obj, hst, f, v, c, e, t))))
            return ec;
        delete v;
    }

    Fpu::operator delete (f, pd->fpu_cache);

    s = Status::MEM_OBJ;

    return nullptr;
}

void Ec::adjust_offset_ticks (uint64 t)
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

    if (EXPECT_FALSE (h & (Hazard::ILLEGAL | Hazard::RECALL | Hazard::SLEEP | Hazard::SCHED))) {

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
            Vmcs::write (Vmcs::Encoding::TSC_OFFSET, regs.exc.offset_tsc);
        } else
            regs.vmcb->tsc_offset = regs.exc.offset_tsc;
    }

    if (EXPECT_FALSE (h & Hazard::FPU))
        Cpu::hazard & Hazard::FPU ? Fpu::disable() : Fpu::enable();

    if (EXPECT_FALSE (h & Hazard::BOOT_HST)) {
        Cpu::hazard &= ~Hazard::BOOT_HST;
        trace (TRACE_PERF, "TIME: First HEC: %llums", Stc::ticks_to_ms (Timer::time() - *reinterpret_cast<uintptr_t *>(Kmem::sym_to_virt (&__boot_ts))));
    }

    if (EXPECT_FALSE (h & Hazard::BOOT_GST)) {
        Cpu::hazard &= ~Hazard::BOOT_GST;
        trace (TRACE_PERF, "TIME: First GEC: %llums", Stc::ticks_to_ms (Timer::time() - *reinterpret_cast<uintptr_t *>(Kmem::sym_to_virt (&__boot_ts))));
    }
}

void Ec_arch::ret_user_hypercall (Ec *const self)
{
    auto const h { (Cpu::hazard ^ self->regs.hazard) & (Hazard::ILLEGAL | Hazard::RECALL | Hazard::FPU | Hazard::BOOT_HST | Hazard::RCU | Hazard::SLEEP | Hazard::SCHED) };
    if (EXPECT_FALSE (h))
        self->handle_hazard (h, ret_user_hypercall);

    Cet::ss_deactivate();

    asm volatile ("lea %0, %%rsp;" EXPAND (LOAD_GPR) "mov %%r11, %%rsp; mov %1, %%r11; sysretq" : : "m" (self->exc_regs()), "i" (RFL_IF | RFL_1) : "memory");

    UNREACHED;
}

void Ec_arch::ret_user_exception (Ec *const self)
{
    auto const h { (Cpu::hazard ^ self->regs.hazard) & (Hazard::ILLEGAL | Hazard::RECALL | Hazard::FPU | Hazard::BOOT_HST | Hazard::RCU | Hazard::SLEEP | Hazard::SCHED) };
    if (EXPECT_FALSE (h))
        self->handle_hazard (h, ret_user_exception);

    Cet::ss_unwind();

    asm volatile ("lea %0, %%rsp;" EXPAND (LOAD_GPR IRET) : : "m" (self->exc_regs()) : "memory");

    UNREACHED;
}

void Ec_arch::ret_user_vmexit_vmx (Ec *const self)
{
    auto const h { (Cpu::hazard ^ self->regs.hazard) & (Hazard::ILLEGAL | Hazard::RECALL | Hazard::TSC | Hazard::BOOT_GST | Hazard::RCU | Hazard::SLEEP | Hazard::SCHED) };
    if (EXPECT_FALSE (h))
        self->handle_hazard (h, ret_user_vmexit_vmx);

    self->regs.vmcs->make_current();

    auto const gst { self->get_gst() };

    if (EXPECT_FALSE (gst->gtlb.tst (Cpu::id))) {
        gst->gtlb.clr (Cpu::id);
        gst->invalidate();
    }

    if (EXPECT_FALSE (Cr::get_cr2() != self->exc_regs().cr2))
        Cr::set_cr2 (self->exc_regs().cr2);

    asm volatile ("lea %0, %%rsp;"
                  EXPAND (LOAD_GPR)
                  "vmresume;"
                  "vmlaunch;"
                  "lea %1, %%rsp;"
                  "jmp vmx_failure;"
                  : : "m" (self->exc_regs()), "m" (DSTK_TOP) : "memory");

    UNREACHED;
}

void Ec_arch::ret_user_vmexit_svm (Ec *const self)
{
    auto const h { (Cpu::hazard ^ self->regs.hazard) & (Hazard::ILLEGAL | Hazard::RECALL | Hazard::TSC | Hazard::BOOT_GST | Hazard::RCU | Hazard::SLEEP | Hazard::SCHED) };
    if (EXPECT_FALSE (h))
        self->handle_hazard (h, ret_user_vmexit_svm);

    auto const gst { self->get_gst() };

    if (EXPECT_FALSE (gst->gtlb.tst (Cpu::id))) {
        gst->gtlb.clr (Cpu::id);
        self->regs.vmcb->tlb_control = 1;
    }

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
                  : : "m" (self->exc_regs()), "m" (Vmcb::root), "m" (DSTK_TOP) : "memory");

    UNREACHED;
}
