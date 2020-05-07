/*
 * System-Call Interface
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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
#include "counter.hpp"
#include "ec_arch.hpp"
#include "fpu.hpp"
#include "hazards.hpp"
#include "hw.hpp"
#include "interrupt.hpp"
#include "lowlevel.hpp"
#include "pd_kern.hpp"
#include "pt.hpp"
#include "sc.hpp"
#include "sm.hpp"
#include "smmu.hpp"
#include "stdio.hpp"
#include "syscall.hpp"
#include "syscall_tmpl.hpp"
#include "utcb.hpp"

void Ec::recv_kern()
{
    auto ec = current->caller;

    assert (ec);

    assert (current);
    assert (current->utcb);
    assert (current->subtype == Kobject::Subtype::EC_LOCAL);
    assert (current->cont == recv_kern);

    Mtd_arch mtd = static_cast<Sys_ipc_reply *>(current->sys_regs())->mtd_a();

    static_cast<Ec_arch *>(ec)->state_load (mtd);

    Ec_arch::ret_user_hypercall();
}

void Ec::recv_user()
{
    auto ec = current->caller;

    assert (ec);
    assert (ec->utcb);
    assert (ec->subtype != Kobject::Subtype::EC_VCPU);
    assert (ec->cont == Ec_arch::ret_user_hypercall);

    assert (current);
    assert (current->utcb);
    assert (current->subtype == Kobject::Subtype::EC_LOCAL);
    assert (current->cont == recv_user);

    Mtd_user mtd = static_cast<Sys_ipc_reply *>(current->sys_regs())->mtd_u();

    ec->utcb->copy (mtd, current->utcb);

    Ec_arch::ret_user_hypercall();
}

void Ec::reply (void (*c)())
{
    current->cont = c;

    auto ec = current->caller;

    if (EXPECT_TRUE (ec)) {

        assert (current->subtype == Kobject::Subtype::EC_LOCAL);

        if (EXPECT_TRUE (ec->clr_partner()))
            static_cast<Ec_arch *>(ec)->make_current();

        Sc::current->ec->activate();
    }

    Sc::schedule (true);
}

template <void (*C)()>
void Ec::send_msg()
{
    auto r = current->exc_regs();

    auto cap = current->pd->Space_obj::lookup (current->evt + r->ep());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PT, 2)))
        kill ("PT not found");

    auto pt = static_cast<Pt *>(cap.obj());
    auto ec = pt->ec;

    if (EXPECT_FALSE (current->cpu != ec->cpu))
        kill ("PT wrong CPU");

    assert (ec->subtype == Kobject::Subtype::EC_LOCAL);

    if (EXPECT_TRUE (!ec->cont)) {
        current->cont = C;
        current->set_partner (ec);
        ec->cont = recv_kern;
        ec->regs.ip() = pt->ip;
        ec->regs.p0() = pt->id;
        ec->regs.p1() = pt->mtd;
        static_cast<Ec_arch *>(ec)->make_current();
    }

    ec->help (send_msg<C>);

    kill ("IPC Abort");
}

void Ec::sys_ipc_call()
{
    auto r = static_cast<Sys_ipc_call *>(current->sys_regs());

    auto cap = current->pd->Space_obj::lookup (r->pt());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PT, 1)))
        sys_finish<Status::BAD_CAP>();

    auto pt = static_cast<Pt *>(cap.obj());
    auto ec = pt->ec;

    if (EXPECT_FALSE (current->cpu != ec->cpu))
        sys_finish<Status::BAD_CPU>();

    assert (ec->subtype == Kobject::Subtype::EC_LOCAL);

    if (EXPECT_TRUE (!ec->cont)) {
        current->cont = Ec_arch::ret_user_hypercall;
        current->set_partner (ec);
        ec->cont = recv_user;
        ec->regs.ip() = pt->ip;
        ec->regs.p0() = pt->id;
        ec->regs.p1() = r->mtd();
        static_cast<Ec_arch *>(ec)->make_current();
    }

    if (EXPECT_FALSE (r->timeout()))
        sys_finish<Status::TIMEOUT>();

    ec->help (sys_ipc_call);

    sys_finish<Status::ABORTED>();
}

void Ec::sys_ipc_reply()
{
    auto r = static_cast<Sys_ipc_reply *>(current->sys_regs());

    auto ec = current->caller;

    if (EXPECT_TRUE (ec)) {

        if (EXPECT_TRUE (ec->cont == Ec_arch::ret_user_hypercall)) {
            ec->regs.p1() = r->mtd_u();
            current->utcb->copy (r->mtd_u(), ec->utcb);
        }

        else if (EXPECT_FALSE (!static_cast<Ec_arch *>(ec)->state_save (r->mtd_a())))
            ec->set_hazard (HZD_ILLEGAL);
    }

    reply();
}

void Ec::sys_create_pd()
{
    auto r = static_cast<Sys_create_pd *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s PD:%#lx", static_cast<void *>(current), __func__, r->sel());

    auto cap = current->pd->Space_obj::lookup (r->own());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PD, 1))) {
        trace (TRACE_ERROR, "%s: Bad PD CAP (%#lx)", __func__, r->own());
        sys_finish<Status::BAD_CAP>();
    }

    auto pd = Pd::create();

    if (EXPECT_FALSE (!pd)) {
        trace (TRACE_ERROR, "%s: Insufficient MEM", __func__);
        sys_finish<Status::INS_MEM>();
    }

    Status s = current->pd->Space_obj::insert (r->sel(), Capability (pd, cap.prm()));

    if (EXPECT_TRUE (s == Status::SUCCESS))
        sys_finish<Status::SUCCESS>();

    pd->destroy();

    switch (s) {

        default:

        case Status::BAD_CAP:
            trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
            sys_finish<Status::BAD_CAP>();

        case Status::INS_MEM:
            trace (TRACE_ERROR, "%s: Insufficient MEM for CAP (%#lx)", __func__, r->sel());
            sys_finish<Status::INS_MEM>();
    }
}

void Ec::sys_create_ec()
{
    auto r = static_cast<Sys_create_ec *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s EC:%#lx PD:%#lx CPU:%#x UTCB:%#lx SP:%#lx EB:%#lx", static_cast<void *>(current), __func__, r->sel(), r->own(), r->cpu(), r->utcb(), r->sp(), r->eb());

    if (EXPECT_FALSE (r->cpu() >= Cpu::online)) {
        trace (TRACE_ERROR, "%s: Invalid CPU (%u)", __func__, r->cpu());
        sys_finish<Status::BAD_CPU>();
    }

    if (EXPECT_FALSE (r->utcb() >= Space_mem::num << PAGE_BITS)) {
        trace (TRACE_ERROR, "%s: Invalid UTCB address (%#lx)", __func__, r->utcb());
        sys_finish<Status::BAD_PAR>();
    }

    if (EXPECT_FALSE (r->vcpu() && !vcpu_supported())) {
        trace (TRACE_ERROR, "%s: VCPUs not supported", __func__);
        sys_finish<Status::BAD_FTR>();
    }

    auto cap = current->pd->Space_obj::lookup (r->own());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PD, 2))) {
        trace (TRACE_ERROR, "%s: Bad PD CAP (%#lx)", __func__, r->own());
        sys_finish<Status::BAD_CAP>();
    }

    auto pd = static_cast<Pd *>(cap.obj());

    auto ec = r->vcpu() ? Ec::create (pd, r->fpu(), r->cpu(), r->eb()) :
                          Ec::create (pd, r->fpu(), r->cpu(), r->eb(), r->utcb(), r->sp(), r->glb() ? send_msg<Ec_arch::ret_user_exception> : nullptr);

    if (EXPECT_FALSE (!ec)) {
        trace (TRACE_ERROR, "%s: Insufficient MEM", __func__);
        sys_finish<Status::INS_MEM>();
    }

    Status s = current->pd->Space_obj::insert (r->sel(), Capability (ec, 0x1f));

    if (EXPECT_TRUE (s == Status::SUCCESS))
        sys_finish<Status::SUCCESS>();

    ec->destroy();

    switch (s) {

        default:

        case Status::BAD_CAP:
            trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
            sys_finish<Status::BAD_CAP>();

        case Status::INS_MEM:
            trace (TRACE_ERROR, "%s: Insufficient MEM for CAP (%#lx)", __func__, r->sel());
            sys_finish<Status::INS_MEM>();
    }
}

void Ec::sys_create_sc()
{
    auto r = static_cast<Sys_create_sc *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s SC:%#lx EC:%#lx P:%u Q:%u", static_cast<void *>(current), __func__, r->sel(), r->ec(), r->prio(), r->quantum());

    auto cap = current->pd->Space_obj::lookup (r->own());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PD, 3))) {
        trace (TRACE_ERROR, "%s: Bad PD CAP (%#lx)", __func__, r->own());
        sys_finish<Status::BAD_CAP>();
    }

    cap = current->pd->Space_obj::lookup (r->ec());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::EC, 3))) {
        trace (TRACE_ERROR, "%s: Bad EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Status::BAD_CAP>();
    }

    auto ec = static_cast<Ec *>(cap.obj());

    if (EXPECT_FALSE (ec->subtype == Kobject::Subtype::EC_LOCAL)) {
        trace (TRACE_ERROR, "%s: Cannot bind SC", __func__);
        sys_finish<Status::BAD_CAP>();
    }

    if (EXPECT_FALSE (!r->prio() || !r->quantum())) {
        trace (TRACE_ERROR, "%s: Invalid prio/quantum", __func__);
        sys_finish<Status::BAD_PAR>();
    }

    auto sc = Sc::create (ec->cpu, ec, r->prio(), r->quantum());

    if (EXPECT_FALSE (!sc)) {
        trace (TRACE_ERROR, "%s: Insufficient MEM", __func__);
        sys_finish<Status::INS_MEM>();
    }

    Status s = current->pd->Space_obj::insert (r->sel(), Capability (sc, 0x1f));

    if (EXPECT_TRUE (s == Status::SUCCESS)) {
        sc->remote_enqueue();
        sys_finish<Status::SUCCESS>();
    }

    sc->destroy();

    switch (s) {

        default:

        case Status::BAD_CAP:
            trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
            sys_finish<Status::BAD_CAP>();

        case Status::INS_MEM:
            trace (TRACE_ERROR, "%s: Insufficient MEM for CAP (%#lx)", __func__, r->sel());
            sys_finish<Status::INS_MEM>();
    }
}

void Ec::sys_create_pt()
{
    auto r = static_cast<Sys_create_pt *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s PT:%#lx EC:%#lx IP:%#lx", static_cast<void *>(current), __func__, r->sel(), r->ec(), r->ip());

    auto cap = current->pd->Space_obj::lookup (r->own());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PD, 2))) {
        trace (TRACE_ERROR, "%s: Bad PD CAP (%#lx)", __func__, r->own());
        sys_finish<Status::BAD_CAP>();
    }

    cap = current->pd->Space_obj::lookup (r->ec());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::EC, 2))) {
        trace (TRACE_ERROR, "%s: Bad EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Status::BAD_CAP>();
    }

    auto ec = static_cast<Ec *>(cap.obj());

    if (EXPECT_FALSE (ec->subtype != Kobject::Subtype::EC_LOCAL)) {
        trace (TRACE_ERROR, "%s: Cannot bind PT", __func__);
        sys_finish<Status::BAD_CAP>();
    }

    auto pt = Pt::create (ec, r->ip());

    if (EXPECT_FALSE (!pt)) {
        trace (TRACE_ERROR, "%s: Insufficient MEM", __func__);
        sys_finish<Status::INS_MEM>();
    }

    Status s = current->pd->Space_obj::insert (r->sel(), Capability (pt, 0x1f));

    if (EXPECT_TRUE (s == Status::SUCCESS))
        sys_finish<Status::SUCCESS>();

    pt->destroy();

    switch (s) {

        default:

        case Status::BAD_CAP:
            trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
            sys_finish<Status::BAD_CAP>();

        case Status::INS_MEM:
            trace (TRACE_ERROR, "%s: Insufficient MEM for CAP (%#lx)", __func__, r->sel());
            sys_finish<Status::INS_MEM>();
    }
}

void Ec::sys_create_sm()
{
    auto r = static_cast<Sys_create_sm *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s SM:%#lx CNT:%llu", static_cast<void *>(current), __func__, r->sel(), r->cnt());

    auto cap = current->pd->Space_obj::lookup (r->own());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PD, 2))) {
        trace (TRACE_ERROR, "%s: Bad PD CAP (%#lx)", __func__, r->own());
        sys_finish<Status::BAD_CAP>();
    }

    auto sm = Sm::create (r->cnt());

    if (EXPECT_FALSE (!sm)) {
        trace (TRACE_ERROR, "%s: Insufficient MEM", __func__);
        sys_finish<Status::INS_MEM>();
    }

    Status s = current->pd->Space_obj::insert (r->sel(), Capability (sm, 0x1f));

    if (EXPECT_TRUE (s == Status::SUCCESS))
        sys_finish<Status::SUCCESS>();

    sm->destroy();

    switch (s) {

        default:

        case Status::BAD_CAP:
            trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
            sys_finish<Status::BAD_CAP>();

        case Status::INS_MEM:
            trace (TRACE_ERROR, "%s: Insufficient MEM for CAP (%#lx)", __func__, r->sel());
            sys_finish<Status::INS_MEM>();
    }
}

void Ec::sys_ctrl_pd()
{
    auto r = static_cast<Sys_ctrl_pd *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s SPD:%#lx DPD:%#lx ST:%u SI:%u SRC:%#lx DST:%#lx ORD:%u PMM:%#x CA:%u SH:%u", static_cast<void *>(current), __func__, r->spd(), r->dpd(), static_cast<unsigned>(r->st()), static_cast<unsigned>(r->si()), r->src(), r->dst(), r->ord(), r->pmm(), static_cast<unsigned>(r->ca()), static_cast<unsigned>(r->sh()));

    auto cap = current->pd->Space_obj::lookup (r->spd());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PD, 0))) {
        trace (TRACE_ERROR, "%s: Bad SRC PD CAP (%#lx)", __func__, r->spd());
        sys_finish<Status::BAD_CAP>();
    }

    auto src = static_cast<Pd *>(cap.obj());

    cap = current->pd->Space_obj::lookup (r->dpd());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PD, 0))) {
        trace (TRACE_ERROR, "%s: Bad DST PD CAP (%#lx)", __func__, r->dpd());
        sys_finish<Status::BAD_CAP>();
    }

    auto dst = static_cast<Pd *>(cap.obj());

    if (EXPECT_FALSE (dst == &Pd_kern::nova())) {
        trace (TRACE_ERROR, "%s: Bad DST PD CAP (%#lx)", __func__, r->dpd());
        sys_finish<Status::BAD_CAP>();
    }

    if (EXPECT_FALSE ((r->src() | r->dst()) & ((1UL << r->ord()) - 1))) {
        trace (TRACE_ERROR, "%s: Unaligned address", __func__);
        sys_finish<Status::BAD_PAR>();
    }

    switch (r->st()) {

        case Space::Type::OBJ:
            if (dst->update_space_obj (src, r->src(), r->dst(), r->ord(), r->pmm()))
                sys_finish<Status::SUCCESS>();
            break;

        case Space::Type::MEM:
            if (dst->update_space_mem (src, r->src(), r->dst(), r->ord(), r->pmm(), r->ca(), r->sh(), r->si()))
                sys_finish<Status::SUCCESS>();
            break;

        case Space::Type::PIO:
            if (dst->update_space_pio (src, r->src(), r->src(), r->ord(), r->pmm()))
                sys_finish<Status::SUCCESS>();
            break;
    }

    sys_finish<Status::BAD_PAR>();
}

void Ec::sys_ctrl_ec()
{
    auto r = static_cast<Sys_ctrl_ec *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s EC:%#lx (%c)", static_cast<void *>(current), __func__, r->ec(), r->strong() ? 'S' : 'W');

    auto cap = current->pd->Space_obj::lookup (r->ec());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::EC, 0))) {
        trace (TRACE_ERROR, "%s: Bad EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Status::BAD_CAP>();
    }

    auto ec = static_cast<Ec *>(cap.obj());

    if (!(__atomic_load_n (&ec->hazard, __ATOMIC_RELAXED) & HZD_RECALL)) {

        ec->set_hazard (HZD_RECALL);

        if (Cpu::id != ec->cpu && Ec::remote_current (ec->cpu) == ec) {
            if (EXPECT_FALSE (r->strong())) {
                Cpu::preempt_enable();
                auto cnt = Counter::req[Interrupt::Request::RKE].get (ec->cpu);
                Interrupt::send_cpu (ec->cpu, Interrupt::Request::RKE);
                while (Counter::req[Interrupt::Request::RKE].get (ec->cpu) == cnt)
                    pause();
                Cpu::preempt_disable();
            } else
                Interrupt::send_cpu (ec->cpu, Interrupt::Request::RKE);
        }
    }

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_ctrl_sc()
{
    auto r = static_cast<Sys_ctrl_sc *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s SC:%#lx", static_cast<void *>(current), __func__, r->sc());

    auto cap = current->pd->Space_obj::lookup (r->sc());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::SC, 0))) {
        trace (TRACE_ERROR, "%s: Bad SC CAP (%#lx)", __func__, r->sc());
        sys_finish<Status::BAD_CAP>();
    }

    auto sc = static_cast<Sc *>(cap.obj());

    r->set_time_ticks (sc->time);

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_ctrl_pt()
{
    auto r = static_cast<Sys_ctrl_pt *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s PT:%#lx ID:%#lx MTD:%#x", static_cast<void *>(current), __func__, r->pt(), r->id(), static_cast<unsigned>(r->mtd()));

    auto cap = current->pd->Space_obj::lookup (r->pt());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PT, 0))) {
        trace (TRACE_ERROR, "%s: Bad PT CAP (%#lx)", __func__, r->pt());
        sys_finish<Status::BAD_CAP>();
    }

    auto pt = static_cast<Pt *>(cap.obj());

    pt->set_id (r->id());
    pt->set_mtd (r->mtd());

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_ctrl_sm()
{
    auto r = static_cast<Sys_ctrl_sm *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s SM:%#lx OP:%u", static_cast<void *>(current), __func__, r->sm(), r->op());

    auto cap = current->pd->Space_obj::lookup (r->sm());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::SM, r->op()))) {
        trace (TRACE_ERROR, "%s: Bad SM CAP (%#lx)", __func__, r->sm());
        sys_finish<Status::BAD_CAP>();
    }

    auto sm = static_cast<Sm *>(cap.obj());

    if (r->op()) {          // Down

        auto id = sm->id;

        if (id != ~0U) {

            if (Interrupt::int_table[id].cpu != Cpu::id) {
                trace (TRACE_ERROR, "%s: Invalid CPU (%u)", __func__, Cpu::id);
                sys_finish<Status::BAD_CPU>();
            }

            Interrupt::deactivate (id);
        }

        sm->dn (r->zc(), r->time_ticks());

    } else if (!sm->up())   // Up
        sys_finish<Status::OVRFLOW>();

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_ctrl_hw()
{
    auto r = static_cast<Sys_ctrl_hw *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s OP:%#x", static_cast<void *>(current), __func__, r->op());

    if (EXPECT_FALSE (current->pd != Pd::root)) {
        trace (TRACE_ERROR, "%s: Not Root PD", __func__);
        sys_finish<Status::BAD_HYP>();
    }

    switch (Hw::ctrl (r->op(), r)) {

        case Status::SUCCESS:
            sys_finish<Status::SUCCESS>();

        case Status::BAD_FTR:
            sys_finish<Status::BAD_FTR>();

        default:
            sys_finish<Status::BAD_PAR>();
    }
}

void Ec::sys_assign_int()
{
    auto r = static_cast<Sys_assign_int *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s SM:%#lx CPU:%u FLG:%#x", static_cast<void *>(current), __func__, r->sm(), r->cpu(), r->flags());

    if (EXPECT_FALSE (r->cpu() >= Cpu::online)) {
        trace (TRACE_ERROR, "%s: Invalid CPU (%u)", __func__, r->cpu());
        sys_finish<Status::BAD_CPU>();
    }

    auto cap = current->pd->Space_obj::lookup (r->sm());

    if (EXPECT_FALSE (!cap.validate (Kobject::Type::SM, 4))) {
        trace (TRACE_ERROR, "%s: Bad SM CAP (%#lx)", __func__, r->sm());
        sys_finish<Status::BAD_CAP>();
    }

    auto id = static_cast<Sm *>(cap.obj())->id;

    if (EXPECT_FALSE (id == ~0U)) {
        trace (TRACE_ERROR, "%s: Bad IS CAP (%#lx)", __func__, r->sm());
        sys_finish<Status::BAD_CAP>();
    }

    uint32 msi_addr;
    uint16 msi_data;

    Interrupt::configure (id, r->flags(), r->cpu(), r->dev(), msi_addr, msi_data);

    r->set_msi_addr (msi_addr);
    r->set_msi_data (msi_data);

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_assign_dev()
{
    auto r = static_cast<Sys_assign_dev *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s PD:%#lx SMMU:%#lx SI:%u DEV:%#lx", static_cast<void *>(current), __func__, r->pd(), r->smmu(), static_cast<unsigned>(r->si()), r->dev());

    if (EXPECT_FALSE (current->pd != Pd::root)) {
        trace (TRACE_ERROR, "%s: Not Root PD", __func__);
        sys_finish<Status::BAD_HYP>();
    }

    Smmu *smmu = Smmu::lookup (r->smmu());

    if (EXPECT_FALSE (!smmu)) {
        trace (TRACE_ERROR, "%s: Bad SMMU (%#lx)", __func__, r->smmu());
        sys_finish<Status::BAD_DEV>();
    }

    auto cap = current->pd->Space_obj::lookup (r->pd());

    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PD, 4))) {
        trace (TRACE_ERROR, "%s: Bad PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Status::BAD_CAP>();
    }

    auto pd = static_cast<Pd *>(cap.obj());

    if (!smmu->configure (pd, r->si(), r->dev())) {
        trace (TRACE_ERROR, "%s: Bad Parameter for SI/DEV", __func__);
        sys_finish<Status::BAD_PAR>();
    }

    sys_finish<Status::SUCCESS>();
}

template <Status S, bool T>
void Ec::sys_finish()
{
    if (T)
        current->clr_timeout();

    current->regs.p0() = mword (S);

    Ec_arch::ret_user_hypercall();
}

extern "C"
constexpr void (*syscall[16])() =
{
    &Ec::sys_ipc_call,
    &Ec::sys_ipc_reply,
    &Ec::sys_create_pd,
    &Ec::sys_create_ec,
    &Ec::sys_create_sc,
    &Ec::sys_create_pt,
    &Ec::sys_create_sm,
    &Ec::sys_ctrl_pd,
    &Ec::sys_ctrl_ec,
    &Ec::sys_ctrl_sc,
    &Ec::sys_ctrl_pt,
    &Ec::sys_ctrl_sm,
    &Ec::sys_ctrl_hw,
    &Ec::sys_assign_int,
    &Ec::sys_assign_dev,
    &Ec::sys_finish<Status::BAD_HYP>,
};
