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
#include "ec.hpp"
#include "event.hpp"
#include "hazards.hpp"
#include "sc.hpp"
#include "stdio.hpp"
#include "timer.hpp"

void Ec::set_vmm_info()
{
    assert (current->subtype == Kobject::Subtype::EC_VCPU);
    assert (current->cpu == Cpu::id);

    auto r = current->exc_regs();

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

    auto v = current->vmcb;

    v->el1.sp         = Cpu::ccsidr[0][0]; v->el1.tpidr = Cpu::ccsidr[0][1];
    v->el1.contextidr = Cpu::ccsidr[1][0]; v->el1.elr   = Cpu::ccsidr[1][1];
    v->el1.spsr       = Cpu::ccsidr[2][0]; v->el1.esr   = Cpu::ccsidr[2][1];
    v->el1.far        = Cpu::ccsidr[3][0]; v->el1.afsr0 = Cpu::ccsidr[3][1];
    v->el1.afsr1      = Cpu::ccsidr[4][0]; v->el1.ttbr0 = Cpu::ccsidr[4][1];
    v->el1.ttbr1      = Cpu::ccsidr[5][0]; v->el1.tcr   = Cpu::ccsidr[5][1];
    v->el1.mair       = Cpu::ccsidr[6][0]; v->el1.amair = Cpu::ccsidr[6][1];
    v->el1.vbar       = Cpu::ctr;          v->el1.sctlr = Cpu::clidr;

    v->el2.hcr    = (Vmcb::hcr_hyp1 & ~Vmcb::hcr_hyp0) & ~Cpu::hcr_res0;
    v->el2.vmpidr = Cpu::mpidr;
    v->el2.vpidr  = Cpu::midr;

    UNREACHED;
}

void Ec::handle_hazard (unsigned hzd, void (*func)())
{
    // XXX: Handle other hazards

    if (hzd & HZD_ILLEGAL) {
        current->clr_hazard (HZD_ILLEGAL);
        kill ("Illegal execution state");
    }

    if (hzd & HZD_SCHED) {
        current->cont = func;
        Sc::schedule();
    }

    if (hzd & HZD_RECALL) {
        current->clr_hazard (HZD_RECALL);

        if (func == ret_user_vmexit) {
            current->regs.set_ep (Event::gst_arch + Event::Selector::RECALL);
        } else {
            current->regs.set_ep (Event::hst_arch + Event::Selector::RECALL);
        }
    }

    if (hzd & HZD_FPU)
        Cpu::hazard & HZD_FPU ? Fpu::disable() : Fpu::enable();

    if (hzd & HZD_BOOT_HST) {
        Cpu::hazard &= ~HZD_BOOT_HST;
        trace (TRACE_PERF, "TIME: 1st HEC: %llums", Timer::ticks_to_ms (Timer::time() - Cpu::boot_time));
    }

    if (hzd & HZD_BOOT_GST) {
        Cpu::hazard &= ~HZD_BOOT_GST;
        trace (TRACE_PERF, "TIME: 1st GEC: %llums", Timer::ticks_to_ms (Timer::time() - Cpu::boot_time));
    }
}

// XXX: Possible to unify with ret_user_exception?
void Ec::ret_user_hypercall()
{
    trace (TRACE_CONT, "EC:%p %s to M:%#x IP:%#lx SP:%#lx", static_cast<void *>(current), __func__, current->regs.emode(), current->regs.el2_elr, current->regs.el0_sp);

    auto hzd = (Cpu::hazard ^ current->hazard) & (HZD_RECALL | HZD_ILLEGAL | HZD_FPU | HZD_BOOT_HST | HZD_RCU | HZD_SCHED);
    if (EXPECT_FALSE (hzd))
        handle_hazard (hzd, ret_user_hypercall);

    if (Vmcb::current)
        Vmcb::load_hst();

    current->pd->Space_mem::make_current_hst();

    asm volatile ("mov sp, %0;" EXPAND (LOAD_STATE) "eret; dsb nsh; isb" : : "r" (&current->regs), "m" (current->regs));

    UNREACHED;
}

void Ec::ret_user_exception()
{
    trace (TRACE_CONT, "EC:%p %s to M:%#x IP:%#lx SP:%#lx", static_cast<void *>(current), __func__, current->regs.emode(), current->regs.el2_elr, current->regs.el0_sp);

    auto hzd = (Cpu::hazard ^ current->hazard) & (HZD_RECALL | HZD_ILLEGAL | HZD_FPU | HZD_BOOT_HST | HZD_RCU | HZD_SCHED);
    if (EXPECT_FALSE (hzd))
        handle_hazard (hzd, ret_user_exception);

    if (Vmcb::current)
        Vmcb::load_hst();

    current->pd->Space_mem::make_current_hst();

    asm volatile ("mov sp, %0;" EXPAND (LOAD_STATE) "eret; dsb nsh; isb" : : "r" (&current->regs), "m" (current->regs));

    UNREACHED;
}

void Ec::ret_user_vmexit()
{
    auto hzd = (Cpu::hazard ^ current->hazard) & (HZD_RECALL | HZD_ILLEGAL | HZD_FPU | HZD_BOOT_GST | HZD_RCU | HZD_SCHED);
    if (EXPECT_FALSE (hzd))
        handle_hazard (hzd, ret_user_vmexit);

    trace (TRACE_CONT, "EC:%p %s to M:%#x IP:%#lx", static_cast<void *>(current), __func__, current->regs.emode(), current->regs.el2_elr);

    if (Vmcb::current != current->vmcb)
        current->vmcb->load_gst();

    current->pd->Space_mem::make_current_gst();

    asm volatile ("mov sp, %0;" EXPAND (LOAD_STATE) "eret; dsb nsh; isb" : : "r" (&current->regs), "m" (current->regs));

    UNREACHED;
}
