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
#include "ec.hpp"
#include "hazards.hpp"
#include "interrupt.hpp"
#include "lowlevel.hpp"
#include "pt.hpp"
#include "sc.hpp"
#include "sm.hpp"
#include "smc.hpp"
#include "smmu.hpp"
#include "stdio.hpp"
#include "syscall.hpp"
#include "timer.hpp"

void Ec::recv_kern()
{
    auto ec = current->caller;

    assert (ec);
    assert (ec->cont == ret_user_exception || ec->cont == ret_user_vmexit);

    assert (current);
    assert (current->utcb);
    assert (current->subtype == Kobject::Subtype::EC_LOCAL);
    assert (current->cont == recv_kern);

    current->utcb->load (static_cast<Sys_ipc_reply *>(current->sys_regs())->mtd_a(), ec->exc_regs(), ec->cont == ret_user_vmexit ? ec->vmcb : nullptr);

    ret_user_hypercall();
}

void Ec::recv_user()
{
    auto ec = current->caller;

    assert (ec);
    assert (ec->utcb);
    assert (ec->subtype != Kobject::Subtype::EC_VCPU);
    assert (ec->cont == ret_user_hypercall);

    assert (current);
    assert (current->utcb);
    assert (current->subtype == Kobject::Subtype::EC_LOCAL);
    assert (current->cont == recv_user);

    ec->utcb->copy (current->utcb, static_cast<Sys_ipc_reply *>(current->sys_regs())->mtd_u());

    ret_user_hypercall();
}

void Ec::reply (void (*c)())
{
    current->cont = c;

    auto ec = current->caller;

    if (EXPECT_TRUE (ec)) {

        assert (current->subtype == Kobject::Subtype::EC_LOCAL);

        if (EXPECT_TRUE (ec->clr_partner()))
            ec->make_current();

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
        ec->regs.set_p1 (pt->mtd.val());
        ec->regs.set_p0 (pt->id);
        ec->regs.set_ip (pt->ip);
        ec->make_current();
    }

    ec->help (send_msg<C>);

    kill ("IPC Abort");
}

void Ec::sys_ipc_call()
{
    auto r = static_cast<Sys_ipc_call *>(current->sys_regs());

    auto cap = current->pd->Space_obj::lookup (r->pt());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PT, 1)))
        sys_finish<Sys_regs::BAD_CAP>();

    auto pt = static_cast<Pt *>(cap.obj());
    auto ec = pt->ec;

    if (EXPECT_FALSE (current->cpu != ec->cpu))
        sys_finish<Sys_regs::BAD_CPU>();

    assert (ec->subtype == Kobject::Subtype::EC_LOCAL);

    if (EXPECT_TRUE (!ec->cont)) {
        current->cont = ret_user_hypercall;
        current->set_partner (ec);
        ec->cont = recv_user;
        ec->regs.set_p1 (r->mtd().val());
        ec->regs.set_p0 (pt->id);
        ec->regs.set_ip (pt->ip);
        ec->make_current();
    }

    if (EXPECT_FALSE (r->timeout()))
        sys_finish<Sys_regs::TIMEOUT>();

    ec->help (sys_ipc_call);

    sys_finish<Sys_regs::ABORTED>();
}

void Ec::sys_ipc_reply()
{
    auto r = static_cast<Sys_ipc_reply *>(current->sys_regs());

    auto ec = current->caller;

    if (EXPECT_TRUE (ec)) {

        if (EXPECT_TRUE (ec->cont == ret_user_hypercall)) {
            ec->regs.set_p1 (r->mtd_u().val());
            current->utcb->copy (ec->utcb, r->mtd_u());
        } else if (!current->utcb->save (r->mtd_a(), ec->exc_regs(), ec->cont == ret_user_vmexit ? ec->vmcb : nullptr))
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
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto pd = Pd::create();

    if (!current->pd->Space_obj::insert (r->sel(), Capability (pd, cap.prm()))) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
        pd->destroy();
        sys_finish<Sys_regs::BAD_CAP>();
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_create_ec()
{
    auto r = static_cast<Sys_create_ec *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s EC:%#lx PD:%#lx CPU:%#x UTCB:%#lx SP:%#lx EB:%#lx", static_cast<void *>(current), __func__, r->sel(), r->own(), r->cpu(), r->utcb(), r->sp(), r->eb());

    if (EXPECT_FALSE (r->cpu() >= Cpu::online)) {
        trace (TRACE_ERROR, "%s: Invalid CPU (%u)", __func__, r->cpu());
        sys_finish<Sys_regs::BAD_CPU>();
    }

    if (EXPECT_FALSE (r->utcb() >= USER_ADDR)) {
        trace (TRACE_ERROR, "%s: Invalid address (%#lx)", __func__, r->utcb());
        sys_finish<Sys_regs::BAD_PAR>();
    }

    auto cap = current->pd->Space_obj::lookup (r->own());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PD, 2))) {
        trace (TRACE_ERROR, "%s: Bad PD CAP (%#lx)", __func__, r->own());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto pd = static_cast<Pd *>(cap.obj());

    auto fpu = r->fpu() ? new Fpu : nullptr;

    auto ec = r->vcpu() ? Ec::create (pd, fpu, new Vmcb, r->cpu(), r->eb(), set_vmm_info) :
                          Ec::create (pd, fpu, new Utcb, r->cpu(), r->eb(), r->utcb(), r->sp(), r->glb() ? send_msg<ret_user_exception> : nullptr);

    if (!current->pd->Space_obj::insert (r->sel(), Capability (ec, 0x1f))) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
        ec->destroy();
        sys_finish<Sys_regs::BAD_CAP>();
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_create_sc()
{
    auto r = static_cast<Sys_create_sc *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s SC:%#lx EC:%#lx P:%u Q:%u", static_cast<void *>(current), __func__, r->sel(), r->ec(), r->prio(), r->quantum());

    auto cap = current->pd->Space_obj::lookup (r->own());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PD, 3))) {
        trace (TRACE_ERROR, "%s: Bad PD CAP (%#lx)", __func__, r->own());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    cap = current->pd->Space_obj::lookup (r->ec());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::EC, 3))) {
        trace (TRACE_ERROR, "%s: Bad EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto ec = static_cast<Ec *>(cap.obj());

    if (EXPECT_FALSE (ec->subtype == Kobject::Subtype::EC_LOCAL)) {
        trace (TRACE_ERROR, "%s: Cannot bind SC", __func__);
        sys_finish<Sys_regs::BAD_CAP>();
    }

    if (EXPECT_FALSE (!r->prio() || !r->quantum())) {
        trace (TRACE_ERROR, "%s: Invalid prio/quantum", __func__);
        sys_finish<Sys_regs::BAD_PAR>();
    }

    auto sc = Sc::create (ec->cpu, ec, r->prio(), r->quantum());

    if (!current->pd->Space_obj::insert (r->sel(), Capability (sc, 0x1f))) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
        sc->destroy();
        sys_finish<Sys_regs::BAD_CAP>();
    }

    sc->remote_enqueue();

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_create_pt()
{
    auto r = static_cast<Sys_create_pt *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s PT:%#lx EC:%#lx EIP:%#lx", static_cast<void *>(current), __func__, r->sel(), r->ec(), r->ip());

    auto cap = current->pd->Space_obj::lookup (r->own());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PD, 2))) {
        trace (TRACE_ERROR, "%s: Bad PD CAP (%#lx)", __func__, r->own());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    cap = current->pd->Space_obj::lookup (r->ec());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::EC, 2))) {
        trace (TRACE_ERROR, "%s: Bad EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto ec = static_cast<Ec *>(cap.obj());

    if (EXPECT_FALSE (ec->subtype != Kobject::Subtype::EC_LOCAL)) {
        trace (TRACE_ERROR, "%s: Cannot bind PT", __func__);
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto pt = Pt::create (ec, r->ip());

    if (!current->pd->Space_obj::insert (r->sel(), Capability (pt, 0x1f))) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
        pt->destroy();
        sys_finish<Sys_regs::BAD_CAP>();
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_create_sm()
{
    auto r = static_cast<Sys_create_sm *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s SM:%#lx CNT:%lu", static_cast<void *>(current), __func__, r->sel(), r->cnt());

    auto cap = current->pd->Space_obj::lookup (r->own());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PD, 2))) {
        trace (TRACE_ERROR, "%s: Bad PD CAP (%#lx)", __func__, r->own());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto sm = Sm::create (r->cnt());

    if (!current->pd->Space_obj::insert (r->sel(), Capability (sm, 0x1f))) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
        sm->destroy();
        sys_finish<Sys_regs::BAD_CAP>();
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_ctrl_pd()
{
    auto r = static_cast<Sys_ctrl_pd *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s SPD:%#lx DPD:%#lx ST:%u SI:%u SRC:%#lx DST:%#lx ORD:%u PMM:%#x MT:%u SH:%u", static_cast<void *>(current), __func__, r->spd(), r->dpd(), static_cast<unsigned>(r->st()), static_cast<unsigned>(r->si()), r->src(), r->dst(), r->ord(), r->pmm(), static_cast<unsigned>(r->ca()), static_cast<unsigned>(r->sh()));

    auto cap = current->pd->Space_obj::lookup (r->spd());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PD, 0))) {
        trace (TRACE_ERROR, "%s: Bad SRC PD CAP (%#lx)", __func__, r->spd());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto src = static_cast<Pd *>(cap.obj());

    cap = current->pd->Space_obj::lookup (r->dpd());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PD, 0))) {
        trace (TRACE_ERROR, "%s: Bad DST PD CAP (%#lx)", __func__, r->dpd());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto dst = static_cast<Pd *>(cap.obj());

    if (EXPECT_FALSE (dst == &Pd::kern)) {
        trace (TRACE_ERROR, "%s: Bad DST PD CAP (%#lx)", __func__, r->dpd());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    if (EXPECT_FALSE ((r->src() | r->dst()) & ((1UL << r->ord()) - 1))) {
        trace (TRACE_ERROR, "%s: Unaligned address", __func__);
        sys_finish<Sys_regs::BAD_PAR>();
    }

    switch (r->st()) {

        case Space::Type::OBJ:
            dst->update_obj_space (src, r->src(), r->dst(), r->ord(), r->pmm());
            break;

        case Space::Type::MEM:
            dst->update_mem_space (src, r->src(), r->dst(), r->ord(), r->pmm(), r->ca(), r->sh(), r->si());
            break;

        default:
            sys_finish<Sys_regs::BAD_PAR>();
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_ctrl_ec()
{
    auto r = static_cast<Sys_ctrl_ec *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s EC:%#lx (%c)", static_cast<void *>(current), __func__, r->ec(), r->strong() ? 'S' : 'W');

    auto cap = current->pd->Space_obj::lookup (r->ec());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::EC, 0))) {
        trace (TRACE_ERROR, "%s: Bad EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto ec = static_cast<Ec *>(cap.obj());

    if (!(ec->hazard & HZD_RECALL)) {

        ec->set_hazard (HZD_RECALL);

        if (Cpu::id != ec->cpu && Ec::remote_current (ec->cpu) == ec) {
            if (EXPECT_FALSE (r->strong())) {
                auto cnt = Counter::remote_sgi_count (ec->cpu, Interrupt::Sgi::RKE);
                Interrupt::send_sgi (ec->cpu, Interrupt::Sgi::RKE);
                while (Counter::remote_sgi_count (ec->cpu, Interrupt::Sgi::RKE) == cnt)
                    pause();
            } else
                Interrupt::send_sgi (ec->cpu, Interrupt::Sgi::RKE);
        }
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_ctrl_sc()
{
    auto r = static_cast<Sys_ctrl_sc *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s SC:%#lx", static_cast<void *>(current), __func__, r->sc());

    auto cap = current->pd->Space_obj::lookup (r->sc());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::SC, 0))) {
        trace (TRACE_ERROR, "%s: Bad SC CAP (%#lx)", __func__, r->sc());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto sc = static_cast<Sc *>(cap.obj());

    r->set_time_ticks (sc->time);

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_ctrl_pt()
{
    auto r = static_cast<Sys_ctrl_pt *>(current->sys_regs());

    auto cap = current->pd->Space_obj::lookup (r->pt());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PT, 0))) {
        trace (TRACE_ERROR, "%s: Bad PT CAP (%#lx)", __func__, r->pt());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto pt = static_cast<Pt *>(cap.obj());

    pt->set_id (r->id());
    pt->set_mtd (r->mtd());

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_ctrl_sm()
{
    auto r = static_cast<Sys_ctrl_sm *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s SM:%#lx OP:%u", static_cast<void *>(current), __func__, r->sm(), r->op());

    auto cap = current->pd->Space_obj::lookup (r->sm());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::SM, r->op()))) {
        trace (TRACE_ERROR, "%s: Bad SM CAP (%#lx)", __func__, r->sm());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto sm = static_cast<Sm *>(cap.obj());

    switch (r->op()) {

        case 0:
            if (!sm->up())
                sys_finish<Sys_regs::OVRFLOW>();
            break;

        case 1:
            auto spi = sm->spi;

            if (spi != ~0U) {

                if (Interrupt::int_table[spi].cpu != Cpu::id) {
                    trace (TRACE_ERROR, "%s: Invalid CPU (%u)", __func__, Cpu::id);
                    sys_finish<Sys_regs::BAD_CPU>();
                }

                Interrupt::deactivate_spi (spi);
            }

            sm->dn (r->zc(), r->time_ticks());
            break;
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_ctrl_hw()
{
    auto r = static_cast<Sys_ctrl_hw *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s OP:%#x ARG0:%#x", static_cast<void *>(current), __func__, r->op(), r->arg0());

    if (EXPECT_FALSE (current->pd != &Pd::root)) {
        trace (TRACE_ERROR, "%s: Not Root PD", __func__);
        sys_finish<Sys_regs::BAD_HYP>();
    }

    if (EXPECT_FALSE (r->op() != 0xf)) {
        trace (TRACE_ERROR, "%s: Bad OP (%#x)", __func__, r->op());
        sys_finish<Sys_regs::BAD_PAR>();
    }

    Smc::Service  s = Smc::srv (r->arg0());
    Smc::Function f = Smc::fun (r->arg0());

    // XXX: More sophisticated SMC filtering

    if (EXPECT_FALSE (s != Smc::Service::SIP || !(r->arg0() & BIT (31)))) {
        trace (TRACE_ERROR, "%s: Bad SMC (%#x)", __func__, r->arg0());
        sys_finish<Sys_regs::BAD_PAR>();
    }

    if (EXPECT_FALSE (!Cpu::feature (Cpu::Cpu_feature::EL3))) {
        trace (TRACE_ERROR, "%s: SMC not supported", __func__);
        sys_finish<Sys_regs::BAD_FTR>();
    }

    r->set_res ((r->arg0() & BIT (30) ? Smc::call<uint64> : Smc::call<uint32>)(s, f, r->arg1(), r->arg2(), r->arg3(), r->arg4(), r->arg5(), r->arg6(), 0));

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_assign_int()
{
    auto r = static_cast<Sys_assign_int *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s SM:%#lx CPU:%u MSK:%u TRG:%u GST:%u", static_cast<void *>(current), __func__, r->sm(), r->cpu(), r->msk(), r->trg(), r->gst());

    if (EXPECT_FALSE (r->cpu() >= Cpu::online)) {
        trace (TRACE_ERROR, "%s: Invalid CPU (%u)", __func__, r->cpu());
        sys_finish<Sys_regs::BAD_CPU>();
    }

    auto cap = current->pd->Space_obj::lookup (r->sm());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::SM, 4))) {
        trace (TRACE_ERROR, "%s: Bad SM CAP (%#lx)", __func__, r->sm());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto sm = static_cast<Sm *>(cap.obj());

    unsigned spi = sm->spi;
    if (EXPECT_FALSE (spi == ~0U)) {
        trace (TRACE_ERROR, "%s: Bad IS CAP (%#lx)", __func__, r->sm());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    Interrupt::conf_spi (spi, r->cpu(), r->msk(), r->trg(), r->gst());

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_assign_dev()
{
    auto r = static_cast<Sys_assign_dev *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s PD:%#lx SI:%u CTX:%#x SMG:%#x SID:%#x", static_cast<void *>(current), __func__, r->pd(), static_cast<unsigned>(r->si()), r->ctx(), r->smg(), r->sid());

    if (EXPECT_FALSE (current->pd != &Pd::root)) {
        trace (TRACE_ERROR, "%s: Not Root PD", __func__);
        sys_finish<Sys_regs::BAD_HYP>();
    }

    auto cap = current->pd->Space_obj::lookup (r->pd());
    if (EXPECT_FALSE (!cap.validate (Kobject::Type::PD, 4))) {
        trace (TRACE_ERROR, "%s: Bad PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto pd = static_cast<Pd *>(cap.obj());

    if (!Smmu::configure (r->sid(), r->smg(), r->ctx(), pd, r->si())) {
        trace (TRACE_ERROR, "%s: Bad Parameter for SID/SMG/CTX/SI", __func__);
        sys_finish<Sys_regs::BAD_PAR>();
    }

    sys_finish<Sys_regs::SUCCESS>();
}

template <Sys_regs::Status S, bool T>
void Ec::sys_finish()
{
    if (T)
        current->clr_timeout();

    current->regs.set_status (S);

    ret_user_hypercall();
}

template void Ec::send_msg<Ec::ret_user_vmexit>();
template void Ec::sys_finish<Sys_regs::BAD_HYP>();
