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

// Hyperthread
Ec::Ec (Pd *p, Fpu *f, Utcb *u, unsigned c, mword e, mword a, mword s, void (*x)()) : Kobject (Kobject::Type::EC, x ? Kobject::Subtype::EC_GLOBAL : Kobject::Subtype::EC_LOCAL), cpu (c), evt (e), pd (p), fpu (f), utcb (u), cont (x)
{
    trace (TRACE_CREATE, "EC:%p created - CPU:%u PD:%p %c", static_cast<void *>(this), cpu, static_cast<void *>(pd), subtype == Kobject::Subtype::EC_LOCAL ? 'L' : 'G');

    regs.sp() = s;
    regs.set_ep (Event::hst_arch + Event::Selector::STARTUP);

    pd->Space_mem::update (a, Kmem::ptr_to_phys (utcb), 0, Paging::Permissions (Paging::K | Paging::U | Paging::W | Paging::R), Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);
}

// vCPU
template <>
Ec::Ec (Pd *p, Fpu *f, Vmcb *v, unsigned c, mword e) : Kobject (Kobject::Type::EC, Kobject::Subtype::EC_VCPU), regs (v), cpu (c), evt (e), pd (p), fpu (f), cont (Ec_arch::set_vmm_info)
{
    trace (TRACE_CREATE, "EC:%p created - CPU:%u PD:%p %c", static_cast<void *>(this), cpu, static_cast<void *>(pd), 'V');

    regs.set_ep (Event::gst_arch + Event::Selector::STARTUP);
}

// Hyperthread
Ec *Ec::create (Pd *p, bool fpu, unsigned c, mword e, mword a, mword s, void (*x)())
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

// vCPU
Ec *Ec::create (Pd *p, bool fpu, unsigned c, mword e)
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

void Ec::handle_hazard (unsigned hzd, void (*func)())
{
    // XXX: Handle other hazards

    if (hzd & HZD_SCHED) {
        current->cont = func;
        Sc::schedule();
    }

    if (hzd & HZD_ILLEGAL) {
        current->clr_hazard (HZD_ILLEGAL);
        kill ("Illegal execution state");
    }

    if (hzd & HZD_RECALL) {
        current->clr_hazard (HZD_RECALL);

        if (func == Ec_arch::ret_user_vmexit) {
            current->regs.set_ep (Event::gst_arch + Event::Selector::RECALL);
            send_msg<Ec_arch::ret_user_vmexit>();
        } else {
            current->regs.set_ep (Event::hst_arch + Event::Selector::RECALL);
            send_msg<Ec_arch::ret_user_exception>();
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

// XXX: Possible to unify with ret_user_exception?
void Ec_arch::ret_user_hypercall()
{
    trace (TRACE_CONT, "EC:%p %s to M:%#x IP:%#llx SP:%#llx", static_cast<void *>(current), __func__, current->regs.emode(), current->regs.el2.elr, current->regs.el0.sp);

    auto hzd = (Cpu::hazard ^ __atomic_load_n (&current->hazard, __ATOMIC_RELAXED)) & (HZD_ILLEGAL | HZD_RECALL | HZD_FPU | HZD_BOOT_HST | HZD_RCU | HZD_SCHED);
    if (EXPECT_FALSE (hzd))
        handle_hazard (hzd, ret_user_hypercall);

    if (Vmcb::current)
        Vmcb::load_hst();

    current->pd->Space_mem::make_current_hst();

    asm volatile ("mov sp, %0;" EXPAND (LOAD_STATE) "eret; dsb nsh; isb" : : "r" (current->exc_regs()), "m" (*current->exc_regs()));

    UNREACHED;
}

void Ec_arch::ret_user_exception()
{
    trace (TRACE_CONT, "EC:%p %s to M:%#x IP:%#llx SP:%#llx", static_cast<void *>(current), __func__, current->regs.emode(), current->regs.el2.elr, current->regs.el0.sp);

    auto hzd = (Cpu::hazard ^ __atomic_load_n (&current->hazard, __ATOMIC_RELAXED)) & (HZD_ILLEGAL | HZD_RECALL | HZD_FPU | HZD_BOOT_HST | HZD_RCU | HZD_SCHED);
    if (EXPECT_FALSE (hzd))
        handle_hazard (hzd, ret_user_exception);

    if (Vmcb::current)
        Vmcb::load_hst();

    current->pd->Space_mem::make_current_hst();

    asm volatile ("mov sp, %0;" EXPAND (LOAD_STATE) "eret; dsb nsh; isb" : : "r" (current->exc_regs()), "m" (*current->exc_regs()));

    UNREACHED;
}

void Ec_arch::ret_user_vmexit()
{
    auto hzd = (Cpu::hazard ^ __atomic_load_n (&current->hazard, __ATOMIC_RELAXED)) & (HZD_ILLEGAL | HZD_RECALL | HZD_FPU | HZD_BOOT_GST | HZD_RCU | HZD_SCHED);
    if (EXPECT_FALSE (hzd))
        handle_hazard (hzd, ret_user_vmexit);

    trace (TRACE_CONT, "EC:%p %s to M:%#x IP:%#llx", static_cast<void *>(current), __func__, current->regs.emode(), current->regs.el2.elr);

    if (Vmcb::current != current->regs.vmcb)
        current->regs.vmcb->load_gst();

    current->pd->Space_mem::make_current_gst();

    asm volatile ("mov sp, %0;" EXPAND (LOAD_STATE) "eret; dsb nsh; isb" : : "r" (current->exc_regs()), "m" (*current->exc_regs()));

    UNREACHED;
}

void Ec_arch::set_vmm_info()
{
    assert (current->subtype == Kobject::Subtype::EC_VCPU);
    assert (current->cpu == Cpu::id);

    auto r = current->cpu_regs();

    r->r[0]  = Cpu::cpu64_feat[0]; r->r[1] = Cpu::cpu64_feat[1];
    r->r[2]  = Cpu::dbg64_feat[0]; r->r[3] = Cpu::dbg64_feat[1];
    r->r[4]  = Cpu::isa64_feat[0]; r->r[5] = Cpu::isa64_feat[1];
    r->r[6]  = Cpu::mem64_feat[0]; r->r[7] = Cpu::mem64_feat[1];
    r->r[8]  = Cpu::mem64_feat[2]; r->r[9] = Cpu::sve64_feat[0];

    r->r[16] = uint64 (Cpu::cpu32_feat[1]) << 32 | Cpu::cpu32_feat[0];
    r->r[17] = uint64 (Cpu::dbg32_feat[0]) << 32 | Cpu::cpu32_feat[2];
    r->r[18] = uint64 (Cpu::isa32_feat[0]) << 32 | Cpu::dbg32_feat[1];
    r->r[19] = uint64 (Cpu::isa32_feat[2]) << 32 | Cpu::isa32_feat[1];
    r->r[20] = uint64 (Cpu::isa32_feat[4]) << 32 | Cpu::isa32_feat[3];
    r->r[21] = uint64 (Cpu::isa32_feat[6]) << 32 | Cpu::isa32_feat[5];
    r->r[22] = uint64 (Cpu::mem32_feat[1]) << 32 | Cpu::mem32_feat[0];
    r->r[23] = uint64 (Cpu::mem32_feat[3]) << 32 | Cpu::mem32_feat[2];
    r->r[24] = uint64 (Cpu::mem32_feat[5]) << 32 | Cpu::mem32_feat[4];

    r->r[29] = uint64 (Cpu::mfp32_feat[1]) << 32 | Cpu::mfp32_feat[0];
    r->r[30] =                                     Cpu::mfp32_feat[2];

    auto v = r->vmcb;

    v->el2.hcr    = (Vmcb::hcr_hyp1 & ~Vmcb::hcr_hyp0) & ~Cpu::hcr_res0;
    v->el2.vmpidr = Cpu::mpidr;
    v->el2.vpidr  = Cpu::midr;

    send_msg<ret_user_vmexit>();
}
