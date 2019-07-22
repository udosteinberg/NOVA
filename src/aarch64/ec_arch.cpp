/*
 * Execution Context (EC)
 *
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

#include "assert.hpp"
#include "cpu.hpp"
#include "ec_arch.hpp"
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

// User Thread
Ec::Ec (Pd *p, Fpu *f, Utcb *u, unsigned c, unsigned long e, uintptr_t a, uintptr_t s, cont_t x) : Kobject (Kobject::Type::EC, x ? Kobject::Subtype::EC_GLOBAL : Kobject::Subtype::EC_LOCAL), cpu (c), evt (e), pd (p), fpu (f), utcb (u), cont (x)
{
    trace (TRACE_CREATE, "EC:%p created - CPU:%u PD:%p %c", static_cast<void *>(this), cpu, static_cast<void *>(pd), subtype == Kobject::Subtype::EC_LOCAL ? 'L' : 'G');

    regs.sp() = s;
    regs.set_ep (Event::hst_arch + Event::Selector::STARTUP);

    pd->Space_mem::update (a, Kmem::ptr_to_phys (utcb), 0, Paging::Permissions (Paging::K | Paging::U | Paging::W | Paging::R), Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);
}

// Virtual CPU
template <>
Ec::Ec (Pd *p, Fpu *f, Vmcb *v, unsigned c, unsigned long e) : Kobject (Kobject::Type::EC, Kobject::Subtype::EC_VCPU), regs (v), cpu (c), evt (e), pd (p), fpu (f), cont (Ec_arch::set_vmm_info)
{
    trace (TRACE_CREATE, "EC:%p created - CPU:%u PD:%p %c", static_cast<void *>(this), cpu, static_cast<void *>(pd), 'V');

    regs.set_ep (Event::gst_arch + Event::Selector::STARTUP);
}

// User Thread
Ec *Ec::create (Pd *p, bool fpu, unsigned c, unsigned long e, uintptr_t a, uintptr_t s, cont_t x)
{
    Ec *ec;
    auto f = fpu ? new Fpu : nullptr;
    auto u = new Utcb;

    if (EXPECT_TRUE ((!fpu || f) && u && (ec = new Ec (p, f, u, c, e, a, s, x))))
        return ec;

    delete u;
    delete f;

    return nullptr;
}

// Virtual CPU
Ec *Ec::create (Pd *p, bool fpu, unsigned c, unsigned long e)
{
    Ec *ec;
    auto f = fpu ? new Fpu : nullptr;
    auto v = new Vmcb;

    if (EXPECT_TRUE ((!fpu || f) && v && (ec = new Ec (p, f, v, c, e))))
        return ec;

    delete v;
    delete f;

    return nullptr;
}

bool Ec::vcpu_supported()
{
    return true;
}

void Ec::adjust_offset_ticks (uint64 t MAYBE_UNUSED)
{
#ifdef USE_VCPU_TIME_OFFSET
    if (subtype == Kobject::Subtype::EC_VCPU)
        regs.vmcb->tmr.cntvoff += t;
#endif
}

void Ec::handle_hazard (unsigned hzd, cont_t func)
{
    // XXX: Handle other hazards

    if (hzd & HZD_SCHED) {
        cont = func;
        Sc::schedule();
    }

    if (hzd & HZD_ILLEGAL) {
        clr_hazard (HZD_ILLEGAL);
        kill ("Illegal execution state");
    }

    if (hzd & HZD_RECALL) {
        clr_hazard (HZD_RECALL);

        if (func == Ec_arch::ret_user_vmexit) {
            regs.set_ep (Event::gst_arch + Event::Selector::RECALL);
        } else {
            regs.set_ep (Event::hst_arch + Event::Selector::RECALL);
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
    assert (self->subtype == Kobject::Subtype::EC_VCPU);
    assert (self->cpu == Cpu::id);

    auto r = self->cpu_regs();
    auto v = r->vmcb;

    Cpu::fill_info_regs (v->el2.hcr, v->el2.vpidr, v->el2.vmpidr, r->r);

    UNREACHED;
}
