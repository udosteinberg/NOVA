/*
 * Execution Context (EC)
 *
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

#include "assert.hpp"
#include "cpu.hpp"
#include "ec_arch.hpp"
#include "entry.hpp"
#include "event.hpp"
#include "extern.hpp"
#include "fpu.hpp"
#include "hazards.hpp"
#include "pd.hpp"
#include "sc.hpp"
#include "stdio.hpp"
#include "timer.hpp"
#include "utcb.hpp"
#include "vmcb.hpp"

// Constructor: Kernel Thread
Ec_arch::Ec_arch (Pd *p, unsigned c, cont_t x) : Ec (p, c, x) {}

// Constructor: User Thread
Ec_arch::Ec_arch (Pd *p, Fpu *f, Utcb *u, unsigned c, unsigned long e, uintptr_t a, uintptr_t s, cont_t x) : Ec (p, f, u, c, e, x)
{
    trace (TRACE_CREATE, "EC:%p created (PD:%p CPU:%u UTCB:%p %c)", static_cast<void *>(this), static_cast<void *>(pd), cpu, static_cast<void *>(u), subtype == Kobject::Subtype::EC_LOCAL ? 'L' : 'G');

    regs.sp() = s;
    regs.set_ep (Event::hst_arch + Event::Selector::STARTUP);

    // Ensure PTAB root exists or is created. FIXME: Allocation failure?
    auto ptab = pd->rcpu_hst();
    assert (ptab);

    pd->Space_mem::update (a, Kmem::ptr_to_phys (utcb), 0, Paging::Permissions (Paging::K | Paging::U | Paging::W | Paging::R), Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);
}

// Constructor: Virtual CPU
template <>
Ec_arch::Ec_arch (Pd *p, Fpu *f, Vmcb *v, unsigned c, unsigned long e, bool t) : Ec (p, f, v, c, e, set_vmm_info, t)
{
    trace (TRACE_CREATE, "EC:%p created (PD:%p CPU:%u VMCB:%p %c)", static_cast<void *>(this), static_cast<void *>(pd), cpu, static_cast<void *>(v), subtype == Kobject::Subtype::EC_VCPU_REAL ? 'R' : 'O');

    regs.set_ep (Event::gst_arch + Event::Selector::STARTUP);

    // Ensure PTAB root exists or is created. FIXME: Allocation failure?
    auto ptab = pd->rcpu_gst();
    assert (ptab);
}

// Factory: Virtual CPU
Ec *Ec::create (Pd *p, bool fpu, unsigned c, unsigned long e, bool t)
{
    Ec *ec;
    auto f = fpu ? new Fpu : nullptr;
    auto v = new Vmcb;

    if (EXPECT_TRUE ((!fpu || f) && v && (ec = new Ec_arch (p, f, v, c, e, t))))
        return ec;

    delete v;
    delete f;

    return nullptr;
}

bool Ec::vcpu_supported()
{
    return true;
}

void Ec::adjust_offset_ticks (uint64 t)
{
    if (subtype == Kobject::Subtype::EC_VCPU_OFFS)
        regs.vmcb->tmr.cntvoff += t;
}

void Ec::handle_hazard (unsigned hzd, cont_t func)
{
    // XXX: Handle other hazards

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

        if (func == Ec_arch::ret_user_vmexit) {
            regs.set_ep (Event::gst_arch + Event::Selector::RECALL);
            send_msg<Ec_arch::ret_user_vmexit> (this);
        } else {
            regs.set_ep (Event::hst_arch + Event::Selector::RECALL);
            send_msg<Ec_arch::ret_user_exception> (this);
        }
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
    auto hzd = (Cpu::hazard ^ self->hazard) & (HZD_ILLEGAL | HZD_RECALL | HZD_FPU | HZD_BOOT_HST | HZD_RCU | HZD_SCHED);
    if (EXPECT_FALSE (hzd))
        self->handle_hazard (hzd, ret_user_hypercall);

    trace (TRACE_CONT, "EC:%p %s to M:%#x IP:%#llx SP:%#llx", static_cast<void *>(self), __func__, self->regs.emode(), self->regs.el2.elr, self->regs.el0.sp);

    if (Vmcb::current)
        Vmcb::load_hst();

    self->pd->Space_mem::make_current_hst();

    asm volatile ("mov sp, %0;" EXPAND (LOAD_STATE) "eret; dsb nsh; isb" : : "r" (self->exc_regs()), "m" (*self->exc_regs()));

    UNREACHED;
}

void Ec_arch::ret_user_exception (Ec *const self)
{
    auto hzd = (Cpu::hazard ^ self->hazard) & (HZD_ILLEGAL | HZD_RECALL | HZD_FPU | HZD_BOOT_HST | HZD_RCU | HZD_SCHED);
    if (EXPECT_FALSE (hzd))
        self->handle_hazard (hzd, ret_user_exception);

    trace (TRACE_CONT, "EC:%p %s to M:%#x IP:%#llx SP:%#llx", static_cast<void *>(self), __func__, self->regs.emode(), self->regs.el2.elr, self->regs.el0.sp);

    if (Vmcb::current)
        Vmcb::load_hst();

    self->pd->Space_mem::make_current_hst();

    asm volatile ("mov sp, %0;" EXPAND (LOAD_STATE) "eret; dsb nsh; isb" : : "r" (self->exc_regs()), "m" (*self->exc_regs()));

    UNREACHED;
}

void Ec_arch::ret_user_vmexit (Ec *const self)
{
    auto hzd = (Cpu::hazard ^ self->hazard) & (HZD_ILLEGAL | HZD_RECALL | HZD_FPU | HZD_BOOT_GST | HZD_RCU | HZD_SCHED);
    if (EXPECT_FALSE (hzd))
        self->handle_hazard (hzd, ret_user_vmexit);

    trace (TRACE_CONT, "EC:%p %s to M:%#x IP:%#llx", static_cast<void *>(self), __func__, self->regs.emode(), self->regs.el2.elr);

    if (Vmcb::current != self->regs.vmcb)
        self->regs.vmcb->load_gst();

    self->pd->Space_mem::make_current_gst();

    asm volatile ("mov sp, %0;" EXPAND (LOAD_STATE) "eret; dsb nsh; isb" : : "r" (self->exc_regs()), "m" (*self->exc_regs()));

    UNREACHED;
}

void Ec_arch::set_vmm_info (Ec *const self)
{
    assert (self->is_vcpu());
    assert (self->cpu == Cpu::id);

    auto r = self->cpu_regs();
    auto v = r->vmcb;

    Cpu::fill_info_regs (v->el2.hcr, v->el2.vpidr, v->el2.vmpidr, r->r);

    send_msg<ret_user_vmexit> (self);
}
