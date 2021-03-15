/*
 * System-Call Interface
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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
#include "counter.hpp"
#include "ec_arch.hpp"
#include "interrupt.hpp"
#include "lowlevel.hpp"
#include "pt.hpp"
#include "sm.hpp"
#include "smmu.hpp"
#include "space_dma.hpp"
#include "space_gst.hpp"
#include "space_hst.hpp"
#include "space_msr.hpp"
#include "space_obj.hpp"
#include "space_pio.hpp"
#include "stdio.hpp"
#include "syscall.hpp"
#include "syscall_tmp.hpp"
#include "utcb.hpp"

Ec::cont_t const Ec::syscall[16] =
{
    &sys_ipc_call,
    &sys_ipc_reply,
    &sys_create_pd,
    &sys_create_ec,
    &sys_create_sc,
    &sys_create_pt,
    &sys_create_sm,
    &sys_ctrl_pd,
    &sys_ctrl_ec,
    &sys_ctrl_sc,
    &sys_ctrl_pt,
    &sys_ctrl_sm,
    &sys_ctrl_hw,
    &sys_assign_int,
    &sys_assign_dev,
    &sys_finish<Status::BAD_HYP>,
};

void Ec::recv_kern (Ec *const self)
{
    auto const ec { self->caller };

    assert (ec);

    assert (self);
    assert (self->get_utcb());
    assert (self->subtype == Kobject::Subtype::EC_LOCAL);
    assert (self->cont == recv_kern);

    auto const mtd { Sys_ipc_reply (self->sys_regs()).mtd_a() };

    static_cast<Ec_arch *>(ec)->state_load (self, mtd);

    Ec_arch::ret_user_hypercall (self);
}

void Ec::recv_user (Ec *const self)
{
    auto const ec { self->caller };

    assert (ec);
    assert (ec->get_utcb());
    assert (ec->cont == Ec_arch::ret_user_hypercall);

    assert (self);
    assert (self->get_utcb());
    assert (self->subtype == Kobject::Subtype::EC_LOCAL);
    assert (self->cont == recv_user);

    auto const mtd { Sys_ipc_reply (self->sys_regs()).mtd_u() };

    ec->get_utcb()->copy (mtd, self->get_utcb());

    Ec_arch::ret_user_hypercall (self);
}

void Ec::rendezvous (Ec *const ec, cont_t c, cont_t e, uintptr_t ip, uintptr_t id, uintptr_t mtd)
{
    if (EXPECT_FALSE (ec->cont))
        return;

    cont = c;
    set_partner (ec);

    ec->cont = e;
    ec->exc_regs().ip() = ip;

    Sys_abi abi { ec->sys_regs() };
    abi.p0() = id;
    abi.p1() = mtd;

    static_cast<Ec_arch *>(ec)->make_current();
}

void Ec::reply (cont_t c)
{
    cont = c;

    auto const ec { caller };

    if (EXPECT_TRUE (ec)) {

        assert (subtype == Kobject::Subtype::EC_LOCAL);

        if (EXPECT_TRUE (ec->clr_partner()))
            static_cast<Ec_arch *>(ec)->make_current();

        Scheduler::get_current()->get_ec()->activate();
    }

    Scheduler::schedule (true);
}

template <Ec::cont_t C>
void Ec::send_msg (Ec *const self)
{
    auto r { self->exc_regs() };

    auto const obj { self->regs.get_obj() };
    auto const cpt { obj->lookup (self->evt + r.ep()) };

    if (EXPECT_FALSE (!cpt.validate (Capability::Perm_pt::EVENT)))
        self->kill ("PT not found");

    auto const pt { static_cast<Pt *>(cpt.obj()) };
    auto const ec { pt->get_ec() };

    if (EXPECT_FALSE (self->cpu != ec->cpu))
        self->kill ("PT wrong CPU");

    assert (ec->subtype == Kobject::Subtype::EC_LOCAL);

    self->rendezvous (ec, C, recv_kern, pt->get_ip(), pt->get_id(), pt->get_mtd());

    self->help (ec, send_msg<C>);

    self->kill ("IPC Abort");
}

void Ec::sys_ipc_call (Ec *const self)
{
    Sys_ipc_call r { self->sys_regs() };

    auto const obj { self->regs.get_obj() };
    auto const cpt { obj->lookup (r.pt()) };

    if (EXPECT_FALSE (!cpt.validate (Capability::Perm_pt::CALL)))
        sys_finish<Status::BAD_CAP> (self);

    auto const pt { static_cast<Pt *>(cpt.obj()) };
    auto const ec { pt->get_ec() };

    if (EXPECT_FALSE (self->cpu != ec->cpu))
        sys_finish<Status::BAD_CPU> (self);

    assert (ec->subtype == Kobject::Subtype::EC_LOCAL);

    self->rendezvous (ec, Ec_arch::ret_user_hypercall, recv_user, pt->get_ip(), pt->get_id(), r.mtd());

    if (EXPECT_FALSE (r.timeout()))
        sys_finish<Status::TIMEOUT> (self);

    self->help (ec, sys_ipc_call);

    sys_finish<Status::ABORTED> (self);
}

void Ec::sys_ipc_reply (Ec *const self)
{
    Sys_ipc_reply r { self->sys_regs() };

    auto const ec { self->caller };

    if (EXPECT_TRUE (ec)) {

        if (EXPECT_TRUE (ec->cont == Ec_arch::ret_user_hypercall)) {
            Sys_abi (ec->sys_regs()).p1() = r.mtd_u();
            self->get_utcb()->copy (r.mtd_u(), ec->get_utcb());
        }

        else if (EXPECT_FALSE (!static_cast<Ec_arch *>(ec)->state_save (self, r.mtd_a())))
            ec->regs.hazard.set (Hazard::ILLEGAL);
    }

    self->reply();
}

void Ec::sys_create_pd (Ec *const self)
{
    Sys_create_pd r { self->sys_regs() };

    trace (TRACE_SYSCALL, "EC:%p %s SEL:%#lx PD:%#lx (OP:%u)", static_cast<void *>(self), __func__, r.sel(), r.pd(), r.op());

    auto const obj { self->regs.get_obj() };
    auto const cpd { obj->lookup (r.pd()) };

    if (EXPECT_FALSE (!cpd.validate (Capability::Perm_pd::PD)))
        self->sys_finish_status (Status::BAD_CAP);

    auto const pd { static_cast<Pd *>(cpd.obj()) };

    Status s;

    switch (static_cast<Kobject::Subtype>(r.op())) {
        default: s = Status::BAD_PAR; break;
        case Kobject::Subtype::PD:  Pd::create_pd  (s, obj, r.sel(), cpd.prm()); break;
        case Kobject::Subtype::OBJ: pd->create_obj (s, obj, r.sel()); break;
        case Kobject::Subtype::HST: pd->create_hst (s, obj, r.sel()); break;
        case Kobject::Subtype::GST: pd->create_gst (s, obj, r.sel()); break;
        case Kobject::Subtype::DMA: pd->create_dma (s, obj, r.sel()); break;
        case Kobject::Subtype::PIO: pd->create_pio (s, obj, r.sel()); break;
        case Kobject::Subtype::MSR: pd->create_msr (s, obj, r.sel()); break;
    }

    self->sys_finish_status (s);
}

void Ec::sys_create_ec (Ec *const self)
{
    Sys_create_ec r { self->sys_regs() };

    trace (TRACE_SYSCALL, "EC:%p %s SEL:%#lx PD:%#lx CPU:%#x HVA:%#lx SP:%#lx EVT:%#lx", static_cast<void *>(self), __func__, r.sel(), r.pd(), r.cpu(), r.hva(), r.sp(), r.evt());

    if (EXPECT_FALSE (r.hva() >= Space_hst::selectors() << PAGE_BITS))
        self->sys_finish_status (Status::BAD_PAR);

    if (EXPECT_FALSE (r.cpu() >= Cpu::count))
        self->sys_finish_status (Status::BAD_CPU);

    auto const obj { self->regs.get_obj() };
    auto const cpd { obj->lookup (r.pd()) };

    if (EXPECT_FALSE (!cpd.validate (Capability::Perm_pd::EC)))
        self->sys_finish_status (Status::BAD_CAP);

    Status s;
    Pd::create_ec (s, obj, r.sel(), static_cast<Pd *>(cpd.obj()), r.cpu(), r.evt(), r.sp(), r.hva(), r.flg());

    self->sys_finish_status (s);
}

void Ec::sys_create_sc (Ec *const self)
{
    Sys_create_sc r { self->sys_regs() };

    trace (TRACE_SYSCALL, "EC:%p %s SEL:%#lx PD:%#lx EC:%#lx P:%u B:%u C:%u", static_cast<void *>(self), __func__, r.sel(), r.pd(), r.ec(), r.prio(), r.budget(), r.cos());

    if (EXPECT_FALSE (!r.prio() || !r.budget()))
        self->sys_finish_status (Status::BAD_PAR);

    auto const obj { self->regs.get_obj() };
    auto const cpd { obj->lookup (r.pd()) };
    auto const cec { obj->lookup (r.ec()) };

    if (EXPECT_FALSE (!cpd.validate (Capability::Perm_pd::SC) || !cec.validate (Capability::Perm_ec::BIND_SC)))
        self->sys_finish_status (Status::BAD_CAP);

    auto const ec { static_cast<Ec *>(cec.obj()) };

    if (EXPECT_FALSE (ec->subtype == Kobject::Subtype::EC_LOCAL))
        self->sys_finish_status (Status::BAD_CAP);

    Status s;
    auto const sc { Pd::create_sc (s, obj, r.sel(), ec, ec->cpu, r.budget(), r.prio(), r.cos()) };

    if (EXPECT_TRUE (sc))
        Scheduler::unblock (sc);

    self->sys_finish_status (s);
}

void Ec::sys_create_pt (Ec *const self)
{
    Sys_create_pt r { self->sys_regs() };

    trace (TRACE_SYSCALL, "EC:%p %s SEL:%#lx PD:%#lx EC:%#lx IP:%#lx", static_cast<void *>(self), __func__, r.sel(), r.pd(), r.ec(), r.ip());

    auto const obj { self->regs.get_obj() };
    auto const cpd { obj->lookup (r.pd()) };
    auto const cec { obj->lookup (r.ec()) };

    if (EXPECT_FALSE (!cpd.validate (Capability::Perm_pd::PT) || !cec.validate (Capability::Perm_ec::BIND_PT)))
        self->sys_finish_status (Status::BAD_CAP);

    auto const ec { static_cast<Ec *>(cec.obj()) };

    if (EXPECT_FALSE (ec->subtype != Kobject::Subtype::EC_LOCAL))
        self->sys_finish_status (Status::BAD_CAP);

    Status s;
    Pd::create_pt (s, obj, r.sel(), ec, r.ip());

    self->sys_finish_status (s);
}

void Ec::sys_create_sm (Ec *const self)
{
    Sys_create_sm r { self->sys_regs() };

    trace (TRACE_SYSCALL, "EC:%p %s SEL:%#lx PD:%#lx CNT:%lu", static_cast<void *>(self), __func__, r.sel(), r.pd(), r.cnt());

    auto const obj { self->regs.get_obj() };
    auto const cpd { obj->lookup (r.pd()) };

    if (EXPECT_FALSE (!cpd.validate (Capability::Perm_pd::SM)))
        self->sys_finish_status (Status::BAD_CAP);

    Status s;
    Pd::create_sm (s, obj, r.sel(), r.cnt());

    self->sys_finish_status (s);
}

void Ec::sys_ctrl_pd (Ec *const self)
{
    Sys_ctrl_pd r { self->sys_regs() };

    trace (TRACE_SYSCALL, "EC:%p %s SRC:%#lx DST:%#lx SSB:%#lx DSB:%#lx ORD:%u PMM:%#x", static_cast<void *>(self), __func__, r.src(), r.dst(), r.ssb(), r.dsb(), r.ord(), r.pmm());

    if (EXPECT_FALSE ((r.ssb() | r.dsb()) & (BITN (r.ord()) - 1)))
        self->sys_finish_status (Status::BAD_PAR);

    auto const obj { self->regs.get_obj() };
    auto const cst { obj->lookup (r.src()) };
    auto const cdt { obj->lookup (r.dst()) };

    Kobject::Subtype st, dt;

    if (EXPECT_TRUE (Capability::validate_take_grant (cst, cdt, st, dt))) {

        if (st == Kobject::Subtype::HST) {
            if (static_cast<Space_hst *>(cst.obj()) == &Space_hst::nova && !r.ma().valid())
                self->sys_finish_status (Status::BAD_PAR);
            if (dt == Kobject::Subtype::HST)
                self->sys_finish_status (static_cast<Space_hst *>(cdt.obj())->delegate (static_cast<Space_hst *>(cst.obj()), r.ssb(), r.dsb(), r.ord(), r.pmm(), r.ma()));
            if (dt == Kobject::Subtype::GST)
                self->sys_finish_status (static_cast<Space_gst *>(cdt.obj())->delegate (static_cast<Space_hst *>(cst.obj()), r.ssb(), r.dsb(), r.ord(), r.pmm(), r.ma()));
            if (dt == Kobject::Subtype::DMA)
                self->sys_finish_status (static_cast<Space_dma *>(cdt.obj())->delegate (static_cast<Space_hst *>(cst.obj()), r.ssb(), r.dsb(), r.ord(), r.pmm(), r.ma()));
        }

        else if (st == Kobject::Subtype::OBJ && dt == st)
            self->sys_finish_status (static_cast<Space_obj *>(cdt.obj())->delegate (static_cast<Space_obj *>(cst.obj()), r.ssb(), r.dsb(), r.ord(), r.pmm()));
        else if (st == Kobject::Subtype::PIO && dt == st)
            self->sys_finish_status (static_cast<Space_pio *>(cdt.obj())->delegate (static_cast<Space_pio *>(cst.obj()), r.ssb(), r.dsb(), r.ord(), r.pmm()));
        else if (st == Kobject::Subtype::MSR && dt == st)
            self->sys_finish_status (static_cast<Space_msr *>(cdt.obj())->delegate (static_cast<Space_msr *>(cst.obj()), r.ssb(), r.dsb(), r.ord(), r.pmm()));
    }

    self->sys_finish_status (Status::BAD_CAP);
}

void Ec::sys_ctrl_ec (Ec *const self)
{
    Sys_ctrl_ec r { self->sys_regs() };

    trace (TRACE_SYSCALL, "EC:%p %s EC:%#lx (%c)", static_cast<void *>(self), __func__, r.ec(), r.strong() ? 'S' : 'W');

    auto const obj { self->regs.get_obj() };
    auto const cec { obj->lookup (r.ec()) };

    if (EXPECT_FALSE (!cec.validate (Capability::Perm_ec::CTRL)))
        self->sys_finish_status (Status::BAD_CAP);

    auto const ec { static_cast<Ec *>(cec.obj()) };

    // Strong: Must wait for observation even if the hazard was set already
    if (r.strong()) {

        ec->regs.hazard.set (Hazard::RECALL);

        // Send IPI only if the EC is remote and current on its core
        if (Cpu::id != ec->cpu && Ec::remote_current (ec->cpu) == ec) {
            Cpu::preemption_enable();
            auto cnt { Counter::req[Interrupt::Request::RKE].get (ec->cpu) };
            Interrupt::send_cpu (Interrupt::Request::RKE, ec->cpu);
            while (Counter::req[Interrupt::Request::RKE].get (ec->cpu) == cnt)
                pause();
            Cpu::preemption_disable();
        }

    // Weak: Send IPI only if the hazard was not set already and the EC is remote and current on its core
    } else if (!ec->regs.hazard.tas (Hazard::RECALL) && Cpu::id != ec->cpu && Ec::remote_current (ec->cpu) == ec)
        Interrupt::send_cpu (Interrupt::Request::RKE, ec->cpu);

    self->sys_finish_status (Status::SUCCESS);
}

void Ec::sys_ctrl_sc (Ec *const self)
{
    Sys_ctrl_sc r { self->sys_regs() };

    trace (TRACE_SYSCALL, "EC:%p %s SC:%#lx", static_cast<void *>(self), __func__, r.sc());

    auto const obj { self->regs.get_obj() };
    auto const csc { obj->lookup (r.sc()) };

    if (EXPECT_FALSE (!csc.validate (Capability::Perm_sc::CTRL)))
        self->sys_finish_status (Status::BAD_CAP);

    auto const sc { static_cast<Sc *>(csc.obj()) };

    r.set_time_ticks (sc->get_used());

    self->sys_finish_status (Status::SUCCESS);
}

void Ec::sys_ctrl_pt (Ec *const self)
{
    Sys_ctrl_pt r { self->sys_regs() };

    trace (TRACE_SYSCALL, "EC:%p %s PT:%#lx ID:%#lx MTD:%#x", static_cast<void *>(self), __func__, r.pt(), r.id(), static_cast<unsigned>(r.mtd()));

    auto const obj { self->regs.get_obj() };
    auto const cpt { obj->lookup (r.pt()) };

    if (EXPECT_FALSE (!cpt.validate (Capability::Perm_pt::CTRL)))
        self->sys_finish_status (Status::BAD_CAP);

    auto const pt { static_cast<Pt *>(cpt.obj()) };

    pt->set_id (r.id());
    pt->set_mtd (r.mtd());

    self->sys_finish_status (Status::SUCCESS);
}

void Ec::sys_ctrl_sm (Ec *const self)
{
    Sys_ctrl_sm r { self->sys_regs() };

    trace (TRACE_SYSCALL, "EC:%p %s SM:%#lx OP:%u", static_cast<void *>(self), __func__, r.sm(), r.op());

    auto const obj { self->regs.get_obj() };
    auto const csm { obj->lookup (r.sm()) };

    if (EXPECT_FALSE (!csm.validate (r.op() ? Capability::Perm_sm::CTRL_DN : Capability::Perm_sm::CTRL_UP)))
        self->sys_finish_status (Status::BAD_CAP);

    auto const sm { static_cast<Sm *>(csm.obj()) };

    if (r.op()) {           // Down

        auto const id { sm->get_id() };

        if (id != ~0U) {

            Interrupt::Config cfg { Interrupt::int_table[id].config };

            if (Cpu::id != cfg.cpu())
                self->sys_finish_status (Status::BAD_CPU);

            // Guest-assigned interrupts are deactivated by the guest
            if (!cfg.gst())
                Interrupt::deactivate (id);
        }

        sm->dn (self, r.zc(), r.time_ticks());

    } else if (!sm->up())   // Up
        self->sys_finish_status (Status::OVRFLOW);

    self->sys_finish_status (Status::SUCCESS);
}

void Ec::sys_ctrl_hw (Ec *const self)
{
    Sys_ctrl_hw r { self->sys_regs() };

    trace (TRACE_SYSCALL, "EC:%p %s OP:%u DESC:%#lx", static_cast<void *>(self), __func__, r.op(), r.desc());

    auto const obj { self->regs.get_obj() };

    if (EXPECT_FALSE (obj != Pd::root->get_obj()))
        self->sys_finish_status (Status::BAD_HYP);

    switch (r.op()) {

        default:            // Invalid Operation
            self->sys_finish_status (Status::BAD_PAR);

        case 0:             // S-State Transition
            Acpi_fixed::Transition t { static_cast<uint16_t>(r.desc()) };

            if (EXPECT_FALSE (!Acpi_fixed::supported (t)))
                self->sys_finish_status (Status::BAD_FTR);

            if (EXPECT_FALSE (!Acpi::set_transition (t)))
                self->sys_finish_status (Status::ABORTED);

            Interrupt::send_exc (Interrupt::Request::RKE);

            Cpu::hazard |= Hazard::SLEEP;

            self->sys_finish_status (Status::SUCCESS);
    }
}

void Ec::sys_assign_int (Ec *const self)
{
    Sys_assign_int r { self->sys_regs() };

    trace (TRACE_SYSCALL, "EC:%p %s SM:%#lx CPU:%u FLG:%#x", static_cast<void *>(self), __func__, r.sm(), r.cpu(), r.flg());

    if (EXPECT_FALSE (r.cpu() >= Cpu::count))
        self->sys_finish_status (Status::BAD_CPU);

    auto const obj { self->regs.get_obj() };
    auto const csm { obj->lookup (r.sm()) };

    if (EXPECT_FALSE (!csm.validate (Capability::Perm_sm::ASSIGN)))
        self->sys_finish_status (Status::BAD_CAP);

    assert (static_cast<Sm *>(csm.obj())->get_id() != ~0U);

    uint32_t msi_addr;
    uint16_t msi_data;

    Interrupt::configure (static_cast<Sm *>(csm.obj())->get_id(), Interrupt::Config (r.cpu(), r.dev(), r.flg()), msi_addr, msi_data);

    r.set_msi_addr (msi_addr);
    r.set_msi_data (msi_data);

    self->sys_finish_status (Status::SUCCESS);
}

void Ec::sys_assign_dev (Ec *const self)
{
    Sys_assign_dev r { self->sys_regs() };

    trace (TRACE_SYSCALL, "EC:%p %s DMA:%#lx SMMU:%#lx DAD:%#lx", static_cast<void *>(self), __func__, r.dma(), r.smmu(), r.dad());

    auto const obj { self->regs.get_obj() };

    if (EXPECT_FALSE (obj != Pd::root->get_obj()))
        self->sys_finish_status (Status::BAD_HYP);

    auto const csp { obj->lookup (r.dma()) };

    if (EXPECT_FALSE (!csp.validate (Capability::Perm_sp::ASSIGN, Kobject::Subtype::DMA)))
        self->sys_finish_status (Status::BAD_CAP);

    auto const smmu { Smmu::lookup (r.smmu()) };

    if (EXPECT_FALSE (!smmu))
        self->sys_finish_status (Status::BAD_DEV);

    if (EXPECT_FALSE (!smmu->configure (static_cast<Space_dma *>(csp.obj()), r.dad())))
        self->sys_finish_status (Status::BAD_PAR);

    self->sys_finish_status (Status::SUCCESS);
}

void Ec::sys_finish_status (Status s)
{
    Sys_abi (sys_regs()).p0() = std::to_underlying (s);

    Ec_arch::ret_user_hypercall (this);
}

template <Status S, bool T>
void Ec::sys_finish (Ec *const self)
{
    if (T)
        self->clr_timeout();

    self->sys_finish_status (S);
}
