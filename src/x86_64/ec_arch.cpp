/*
 * Execution Context (EC)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#include "acpi.hpp"
#include "assert.hpp"
#include "cpu.hpp"
#include "ec_arch.hpp"
#include "event.hpp"
#include "extern.hpp"
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

    (x ? regs.rsp : regs.sp()) = s;

    regs.set_ep (Event::hst_arch + Event::Selector::STARTUP);

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
    v->init (pd->Space_pio::bmp_gst(), pd->Space_msr::bmp_gst(), reinterpret_cast<mword>(sys_regs() + 1), cr3, Kmem::ptr_to_phys (ept), pd->vpid());

    assert (regs.vmcs == Vmcs::current);

    regs.tsc_offset = 0;
    regs.fpu_on     = false;
    regs.pio_bitmap = pd->Space_pio::bmp_gst();
    regs.msr_bitmap = pd->Space_msr::bmp_gst();

    regs.set_cr0_msk<Vmcs>();
    regs.set_cr4_msk<Vmcs>();

    regs.set_cr0<Vmcs> (0);
    regs.set_cr4<Vmcs> (0);
    regs.set_exc<Vmcs> (0);

    regs.set_cpu_ctrl0<Vmcs> (0);
    regs.set_cpu_ctrl1<Vmcs> (0);

    // Make VMCS inactive on the creator CPU in preparation for migrating it to its target CPU.
    // This ensures the VMCS data is in memory and the VMCS is not active on more than one CPU.
    regs.vmcs->clear();

    regs.set_ep (Event::gst_arch + Event::Selector::STARTUP);
}

// Constructor: Virtual CPU (SVM)
template <>
Ec_arch::Ec_arch (Pd *p, Fpu *f, Vmcb *v, unsigned c, unsigned long e, bool t) : Ec (p, f, v, c, e, send_msg<ret_user_vmexit_svm>, t)
{
    trace (TRACE_CREATE, "EC:%p created (PD:%p CPU:%u VMCB:%p %c)", static_cast<void *>(this), static_cast<void *>(pd), cpu, static_cast<void *>(v), subtype == Kobject::Subtype::EC_VCPU_REAL  ? 'R' : 'O');

#if 0   // FIXME
    regs.rax = Kmem::ptr_to_phys (regs.vmcb = new Vmcb (pd->Space_pio::walk(), pd->npt.init_root (false)));
#endif

    regs.set_cr0<Vmcb> (0);
    regs.set_cr4<Vmcb> (0);
    regs.set_exc<Vmcb> (0);

    regs.set_cpu_ctrl0<Vmcb> (0);
    regs.set_cpu_ctrl1<Vmcb> (0);

    regs.set_ep (Event::gst_arch + Event::Selector::STARTUP);
}

// Factory: Virtual CPU
Ec *Ec::create (Pd *p, bool fpu, unsigned c, unsigned long e, bool t)
{
    Ec *ec;
    auto f = fpu ? new Fpu : nullptr;

    if (Hip::hip->feature() & Hip::FEAT_VMX) {
        auto v = new Vmcs;
        if (EXPECT_TRUE ((!fpu || f) && v && (ec = new Ec_arch (p, f, v, c, e, t))))
            return ec;
        delete v;
    }

    if (Hip::hip->feature() & Hip::FEAT_SVM) {
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
    return (Hip::hip->feature() & Hip::FEAT_VMX) || (Hip::hip->feature() & Hip::FEAT_SVM);
}

void Ec::adjust_offset_ticks (uint64 t)
{
    if (subtype == Kobject::Subtype::EC_VCPU_OFFS) {
        regs.tsc_offset -= t;
        set_hazard (HZD_TSC);
    }
}

void Ec::handle_hazard (unsigned hzd, cont_t func)
{
    if (hzd & HZD_SLEEP)
        Acpi::sleep();

    if (hzd & HZD_RCU)
        Rcu::quiet();

    if (hzd & HZD_SCHED) {
        cont = func;
        Scheduler::schedule();
    }

    if (hzd & HZD_ILLEGAL) {
        clr_hazard (HZD_ILLEGAL);
        kill ("Illegal execution state");
    }

    if (hzd & HZD_RECALL) {
        clr_hazard (HZD_RECALL);

        if (func == Ec_arch::ret_user_vmexit_vmx) {
            regs.set_ep (Event::gst_arch + Event::Selector::RECALL);
            send_msg<Ec_arch::ret_user_vmexit_vmx> (this);
        }

        if (func == Ec_arch::ret_user_vmexit_svm) {
            regs.set_ep (Event::gst_arch + Event::Selector::RECALL);
            send_msg<Ec_arch::ret_user_vmexit_svm> (this);
        }

        if (func == Ec_arch::ret_user_hypercall)
            static_cast<Ec_arch *>(this)->redirect_to_iret();

        regs.set_ep (Event::hst_arch + Event::Selector::RECALL);
        send_msg<Ec_arch::ret_user_exception> (this);
    }

    if (hzd & HZD_TSC) {
        clr_hazard (HZD_TSC);

        if (func == Ec_arch::ret_user_vmexit_vmx) {
            regs.vmcs->make_current();
            Vmcs::write (Vmcs::Encoding::TSC_OFFSET, regs.tsc_offset);
        } else
            regs.vmcb->tsc_offset = regs.tsc_offset;
    }

    if (hzd & HZD_DS_ES) {
        Cpu::hazard &= ~HZD_DS_ES;
        asm volatile ("mov %0, %%ds; mov %0, %%es" : : "r" (SEL_USER_DATA));
    }

    if (hzd & HZD_FPU)
        Cpu::hazard & HZD_FPU ? Fpu::disable() : Fpu::enable();

    if (hzd & HZD_BOOT_HST) {
        Cpu::hazard &= ~HZD_BOOT_HST;
        trace (TRACE_PERF, "TIME: First HEC: %llums", Timer::ticks_to_ms (Timer::time() - *reinterpret_cast<uintptr_t *>(Kmem::sym_to_virt (&__boot_ts))));
    }

    if (hzd & HZD_BOOT_GST) {
        Cpu::hazard &= ~HZD_BOOT_GST;
        trace (TRACE_PERF, "TIME: First GEC: %llums", Timer::ticks_to_ms (Timer::time() - *reinterpret_cast<uintptr_t *>(Kmem::sym_to_virt (&__boot_ts))));
    }
}

void Ec_arch::ret_user_hypercall (Ec *const self)
{
    auto hzd = (Cpu::hazard ^ self->hazard) & (HZD_ILLEGAL | HZD_RECALL | HZD_FPU | HZD_DS_ES | HZD_BOOT_HST | HZD_RCU | HZD_SLEEP | HZD_SCHED);
    if (EXPECT_FALSE (hzd))
        self->handle_hazard (hzd, ret_user_hypercall);

    asm volatile ("lea %0, %%rsp;" EXPAND (LOAD_GPR RET_USER_HYP) : : "m" (*self->exc_regs()) : "memory");

    UNREACHED;
}

void Ec_arch::ret_user_exception (Ec *const self)
{
    // No need to check HZD_DS_ES because IRET will reload both anyway
    auto hzd = (Cpu::hazard ^ self->hazard) & (HZD_ILLEGAL | HZD_RECALL | HZD_FPU | HZD_BOOT_HST | HZD_RCU | HZD_SLEEP | HZD_SCHED);
    if (EXPECT_FALSE (hzd))
        self->handle_hazard (hzd, ret_user_exception);

    asm volatile ("lea %0, %%rsp;" EXPAND (LOAD_GPR LOAD_SEG RET_USER_EXC) : : "m" (*self->exc_regs()) : "memory");

    UNREACHED;
}

void Ec_arch::ret_user_vmexit_vmx (Ec *const self)
{
    auto hzd = (Cpu::hazard ^ self->hazard) & (HZD_ILLEGAL | HZD_RECALL | HZD_TSC | HZD_BOOT_GST | HZD_RCU | HZD_SLEEP | HZD_SCHED);
    if (EXPECT_FALSE (hzd))
        self->handle_hazard (hzd, ret_user_vmexit_vmx);

    self->regs.vmcs->make_current();

    auto pd = self->pd;
    if (EXPECT_FALSE (pd->gtlb.tst (Cpu::id))) {
        pd->gtlb.clr (Cpu::id);
        pd->ept.invalidate();
    }

    if (EXPECT_FALSE (get_cr2() != self->regs.cr2))
        set_cr2 (self->regs.cr2);

    asm volatile ("lea %0, %%rsp;"
                  EXPAND (LOAD_GPR)
                  "vmresume;"
                  "vmlaunch;"
                  "mov %1, %%rsp;"
                  : : "m" (*self->exc_regs()), "i" (MMAP_CPU_STCK + PAGE_SIZE) : "memory");

    trace (0, "VM entry failed with error %#x", Vmcs::read<uint32> (Vmcs::Encoding::VMX_INST_ERROR));

    self->kill ("VMENTRY");
}

void Ec_arch::ret_user_vmexit_svm (Ec *const self)
{
    auto hzd = (Cpu::hazard ^ self->hazard) & (HZD_ILLEGAL | HZD_RECALL | HZD_TSC | HZD_BOOT_GST | HZD_RCU | HZD_SLEEP | HZD_SCHED);
    if (EXPECT_FALSE (hzd))
        self->handle_hazard (hzd, ret_user_vmexit_svm);

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
                  "mov %2, %%rsp;"
                  "vmload;"
                  "cli;"
                  "stgi;"
                  "jmp svm_handler;"
                  : : "m" (*self->exc_regs()), "m" (Vmcb::root), "i" (MMAP_CPU_STCK + PAGE_SIZE) : "memory");

    UNREACHED;
}
