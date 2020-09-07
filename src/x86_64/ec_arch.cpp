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
#include "hazards.hpp"
#include "hip.hpp"
#include "multiboot.hpp"
#include "pd.hpp"
#include "rcu.hpp"
#include "sc.hpp"
#include "stdio.hpp"
#include "timer.hpp"

// Constructor: Kernel Thread
Ec_arch::Ec_arch (Pd *p, unsigned c, cont_t x) : Ec (p, c, x) {}

// Constructor: User Thread
Ec_arch::Ec_arch (Pd *p, Fpu *f, Utcb *u, unsigned c, unsigned long e, uintptr_t a, uintptr_t s, cont_t x) : Ec (p, f, u, c, e, x)
{
    trace (TRACE_CREATE, "EC:%p created (PD:%p CPU:%u UTCB:%p %c)", static_cast<void *>(this), static_cast<void *>(pd), cpu, static_cast<void *>(u), subtype == Kobject::Subtype::EC_LOCAL ? 'L' : 'G');

    // Make sure we have a PTAB for this CPU in the PD
    pd->Space_mem::init (cpu);

    (x ? exc_regs().rsp : exc_regs().sp()) = s;

    exc_regs().set_ep (Event::hst_arch + Event::Selector::STARTUP);

    pd->Space_mem::update (a, Kmem::ptr_to_phys (utcb), 0, Paging::Permissions (Paging::K | Paging::U | Paging::W | Paging::R), Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);
}

// Constructor: Virtual CPU (VMX)
template <>
Ec_arch::Ec_arch (Pd *p, Fpu *f, Vmcs *v, unsigned c, unsigned long e, bool t) : Ec (p, f, v, c, e, send_msg<ret_user_vmexit_vmx>, t)
{
    trace (TRACE_CREATE, "EC:%p created (PD:%p CPU:%u VMCB:%p %c)", static_cast<void *>(this), static_cast<void *>(pd), cpu, static_cast<void *>(v), subtype == Kobject::Subtype::EC_VCPU_REAL  ? 'R' : 'O');

    // Make sure we have a PTAB for this CPU in the PD
    pd->Space_mem::init (cpu);

    auto hpt = pd->loc[c].root_init (false);
    auto ept = pd->ept.root_init (false);

    // FIXME: Allocation failure?
    assert (hpt && ept);

    auto cr3 = Kmem::ptr_to_phys (hpt) | (Cpu::feature (Cpu::Feature::PCID) ? pd->pcid() : 0);
    v->init (pd->Space_pio::bmp_gst(), pd->Space_msr::bmp_gst(), reinterpret_cast<uintptr_t>(&sys_regs() + 1), cr3, Kmem::ptr_to_phys (ept), pd->vpid());

    assert (regs.vmcs == Vmcs::current);

    exc_regs().offset_tsc = 0;
    exc_regs().intcpt_cr0 = 0;
    exc_regs().intcpt_cr4 = 0;
    exc_regs().intcpt_exc = 0;
    exc_regs().bitmap_pio = pd->Space_pio::bmp_gst();
    exc_regs().bitmap_msr = pd->Space_msr::bmp_gst();
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
Ec_arch::Ec_arch (Pd *p, Fpu *f, Vmcb *v, unsigned c, unsigned long e, bool t) : Ec (p, f, v, c, e, send_msg<ret_user_vmexit_svm>, t)
{
    trace (TRACE_CREATE, "EC:%p created (PD:%p CPU:%u VMCB:%p %c)", static_cast<void *>(this), static_cast<void *>(pd), cpu, static_cast<void *>(v), subtype == Kobject::Subtype::EC_VCPU_REAL  ? 'R' : 'O');

#if 0   // FIXME
    regs.rax = Kmem::ptr_to_phys (regs.vmcb = new Vmcb (pd->Space_pio::walk(), pd->npt.init_root (false)));
#endif

    exc_regs().offset_tsc = 0;
    exc_regs().intcpt_cr0 = 0;
    exc_regs().intcpt_cr4 = 0;
    exc_regs().intcpt_exc = 0;
    exc_regs().bitmap_pio = pd->Space_pio::bmp_gst();
    exc_regs().bitmap_msr = pd->Space_msr::bmp_gst();
    exc_regs().fpu_on     = false;

    regs.svm_set_bmp_exc();
    regs.svm_set_cpu_pri (0);
    regs.svm_set_cpu_sec (0);

    exc_regs().set_ep (Event::gst_arch + Event::Selector::STARTUP);
}

// Factory: Virtual CPU
Ec *Ec::create (Pd *p, bool fpu, unsigned c, unsigned long e, bool t)
{
    Ec *ec;
    auto f = fpu ? new Fpu : nullptr;

    if (Hip::feature (Hip_arch::Feature::VMX)) {
        auto v = new Vmcs;
        if (EXPECT_TRUE ((!fpu || f) && v && (ec = new Ec_arch (p, f, v, c, e, t))))
            return ec;
        delete v;
    }

    if (Hip::feature (Hip_arch::Feature::SVM)) {
        auto v = new Vmcb;
        if (EXPECT_TRUE ((!fpu || f) && v && (ec = new Ec_arch (p, f, v, c, e, t))))
            return ec;
        delete v;
    }

    delete f;

    return nullptr;
}

bool Ec::vcpu_supported()
{
    return Hip::feature (Hip_arch::Feature::VMX) || Hip::feature (Hip_arch::Feature::SVM);
}

void Ec::adjust_offset_ticks (uint64 t)
{
    if (subtype == Kobject::Subtype::EC_VCPU_OFFS) {
        regs.exc.offset_tsc -= t;
        set_hazard (HZD_TSC);
    }
}

void Ec::handle_hazard (unsigned h, cont_t func)
{
    if (h & HZD_RCU)
        Rcu::quiet();

    if (EXPECT_FALSE (h & (HZD_ILLEGAL | HZD_RECALL | HZD_SLEEP | HZD_SCHED))) {

        Cpu::preemption_point();

        if (Cpu::hazard & HZD_SLEEP) {      // Reload
            cont = func;
            Cpu::fini();
        }

        if (Cpu::hazard & HZD_SCHED) {      // Reload
            cont = func;
            Scheduler::schedule();
        }

        if (h & HZD_ILLEGAL)
            kill ("Illegal execution state");

        if (hazard & HZD_RECALL) {          // Reload
            clr_hazard (HZD_RECALL);

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

    if (h & HZD_TSC) {
        clr_hazard (HZD_TSC);

        if (func == Ec_arch::ret_user_vmexit_vmx) {
            regs.vmcs->make_current();
            Vmcs::write (Vmcs::Encoding::TSC_OFFSET, regs.exc.offset_tsc);
        } else
            regs.vmcb->tsc_offset = regs.exc.offset_tsc;
    }

    if (EXPECT_FALSE (h & HZD_FPU))
        Cpu::hazard & HZD_FPU ? Fpu::disable() : Fpu::enable();

    if (EXPECT_FALSE (h & HZD_BOOT_HST)) {
        Cpu::hazard &= ~HZD_BOOT_HST;
        trace (TRACE_PERF, "TIME: First HEC: %llums", Stc::ticks_to_ms (Timer::time() - *reinterpret_cast<uintptr_t *>(Kmem::sym_to_virt (&__boot_ts))));
    }

    if (EXPECT_FALSE (h & HZD_BOOT_GST)) {
        Cpu::hazard &= ~HZD_BOOT_GST;
        trace (TRACE_PERF, "TIME: First GEC: %llums", Stc::ticks_to_ms (Timer::time() - *reinterpret_cast<uintptr_t *>(Kmem::sym_to_virt (&__boot_ts))));
    }
}

void Ec_arch::ret_user_hypercall (Ec *const self)
{
    auto h = (Cpu::hazard ^ self->hazard) & (HZD_ILLEGAL | HZD_RECALL | HZD_FPU | HZD_BOOT_HST | HZD_RCU | HZD_SLEEP | HZD_SCHED);
    if (EXPECT_FALSE (h))
        self->handle_hazard (h, ret_user_hypercall);

    asm volatile ("lea %0, %%rsp;" EXPAND (LOAD_GPR) "mov %%r11, %%rsp; mov %1, %%r11; sysretq" : : "m" (self->exc_regs()), "i" (RFL_IF | RFL_1) : "memory");

    UNREACHED;
}

void Ec_arch::ret_user_exception (Ec *const self)
{
    auto h = (Cpu::hazard ^ self->hazard) & (HZD_ILLEGAL | HZD_RECALL | HZD_FPU | HZD_BOOT_HST | HZD_RCU | HZD_SLEEP | HZD_SCHED);
    if (EXPECT_FALSE (h))
        self->handle_hazard (h, ret_user_exception);

    asm volatile ("lea %0, %%rsp;" EXPAND (LOAD_GPR IRET) : : "m" (self->exc_regs()) : "memory");

    UNREACHED;
}

void Ec_arch::ret_user_vmexit_vmx (Ec *const self)
{
    auto h = (Cpu::hazard ^ self->hazard) & (HZD_ILLEGAL | HZD_RECALL | HZD_TSC | HZD_BOOT_GST | HZD_RCU | HZD_SLEEP | HZD_SCHED);
    if (EXPECT_FALSE (h))
        self->handle_hazard (h, ret_user_vmexit_vmx);

    self->regs.vmcs->make_current();

    auto pd = self->pd;
    if (EXPECT_FALSE (pd->gtlb.tst (Cpu::id))) {
        pd->gtlb.clr (Cpu::id);
        pd->ept.invalidate();
    }

    if (EXPECT_FALSE (get_cr2() != self->exc_regs().cr2))
        set_cr2 (self->exc_regs().cr2);

    Msr::State::make_current (Cpu::msr, self->regs.msr);

    asm volatile ("lea %0, %%rsp;"
                  EXPAND (LOAD_GPR)
                  "vmresume;"
                  "vmlaunch;"
                  "lea %1, %%rsp;"
                  : : "m" (self->exc_regs()), "m" (DSTK_TOP) : "memory");

    Msr::State::make_current (self->regs.msr, Cpu::msr);

    trace (0, "VM entry failed with error %#x", Vmcs::read<uint32> (Vmcs::Encoding::VMX_INST_ERROR));

    self->kill ("VMENTRY");
}

void Ec_arch::ret_user_vmexit_svm (Ec *const self)
{
    auto h = (Cpu::hazard ^ self->hazard) & (HZD_ILLEGAL | HZD_RECALL | HZD_TSC | HZD_BOOT_GST | HZD_RCU | HZD_SLEEP | HZD_SCHED);
    if (EXPECT_FALSE (h))
        self->handle_hazard (h, ret_user_vmexit_svm);

    auto pd = self->pd;
    if (EXPECT_FALSE (pd->gtlb.tst (Cpu::id))) {
        pd->gtlb.clr (Cpu::id);
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
