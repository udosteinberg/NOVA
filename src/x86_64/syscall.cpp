/*
 * System-Call Interface
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "hip.hpp"
#include "interrupt.hpp"
#include "lapic.hpp"
#include "pt.hpp"
#include "sm.hpp"
#include "smmu.hpp"
#include "stdio.hpp"
#include "syscall.hpp"
#include "utcb.hpp"
#include "vectors.hpp"

template <Sys_regs::Status S, bool T>
void Ec::sys_finish()
{
    if (T)
        current->clr_timeout();

    current->regs.set_status (S);
    ret_user_sysexit();
}

void Ec::activate()
{
    Ec *ec = this;

    // XXX: Make the loop preemptible
    for (Sc::ctr_link = 0; ec->partner; ec = ec->partner)
        Sc::ctr_link++;

    if (EXPECT_FALSE (ec->blocked()))
        ec->block_sc();

    ec->make_current();
}

template <void (*C)()>
void Ec::send_msg()
{
    Exc_regs *r = &current->regs;

    auto cap = current->pd->Space_obj::lookup (current->evt + r->dst_portal);
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_pt::EVENT)))
        die ("PT not found");

    auto pt = static_cast<Pt *>(cap.obj());
    auto ec = pt->ec;

    if (EXPECT_FALSE (current->cpu != ec->xcpu))
        die ("PT wrong CPU");

    if (EXPECT_TRUE (!ec->cont)) {
        current->cont = C;
        current->set_partner (ec);
        current->regs.mtd = pt->mtd.val;
        ec->cont = recv_kern;
        ec->regs.set_pt (pt->id);
        ec->regs.set_ip (pt->ip);
        ec->make_current();
    }

    ec->help (send_msg<C>);

    die ("IPC Timeout");
}

void Ec::sys_call()
{
    Sys_call *s = static_cast<Sys_call *>(current->sys_regs());

    auto cap = current->pd->Space_obj::lookup (s->pt());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_pt::CALL)))
        sys_finish<Sys_regs::BAD_CAP>();

    auto pt = static_cast<Pt *>(cap.obj());
    auto ec = pt->ec;

    if (EXPECT_FALSE (current->cpu != ec->xcpu))
        sys_finish<Sys_regs::BAD_CPU>();

    if (EXPECT_TRUE (!ec->cont)) {
        current->cont = ret_user_sysexit;
        current->set_partner (ec);
        ec->cont = recv_user;
        ec->regs.set_pt (pt->id);
        ec->regs.set_ip (pt->ip);
        ec->make_current();
    }

    if (EXPECT_TRUE (!(s->flags() & Sys_call::DISABLE_BLOCKING)))
        ec->help (sys_call);

    sys_finish<Sys_regs::COM_TIM>();
}

void Ec::recv_kern()
{
    Ec *ec = current->rcap;

    bool fpu = false;

    if (ec->cont == ret_user_iret)
        fpu = current->utcb->load_exc (&ec->regs);
    else if (ec->cont == ret_user_vmresume)
        fpu = current->utcb->load_vmx (&ec->regs);
    else if (ec->cont == ret_user_vmrun)
        fpu = current->utcb->load_svm (&ec->regs);

    if (EXPECT_FALSE (fpu))
        ec->transfer_fpu (current);

    ret_user_sysexit();
}

void Ec::recv_user()
{
    Ec *ec = current->rcap;

    ec->utcb->save (current->utcb);

    ret_user_sysexit();
}

void Ec::reply (void (*c)())
{
    current->cont = c;

    if (EXPECT_FALSE (current->glb))
        Sc::schedule (true);

    Ec *ec = current->rcap;

    if (EXPECT_FALSE (!ec || !ec->clr_partner()))
        Sc::current->ec->activate();

    ec->make_current();
}

void Ec::sys_reply()
{
    Ec *ec = current->rcap;

    if (EXPECT_TRUE (ec)) {

        Utcb *src = current->utcb;

        bool fpu = false;

        if (EXPECT_TRUE (ec->cont == ret_user_sysexit))
            src->save (ec->utcb);
        else if (ec->cont == ret_user_iret)
            fpu = src->save_exc (&ec->regs);
        else if (ec->cont == ret_user_vmresume)
            fpu = src->save_vmx (&ec->regs);
        else if (ec->cont == ret_user_vmrun)
            fpu = src->save_svm (&ec->regs);

        if (EXPECT_FALSE (fpu))
            current->transfer_fpu (ec);
    }

    reply();
}

void Ec::sys_create_pd()
{
    Sys_create_pd *r = static_cast<Sys_create_pd *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE PD:%#lx", current, r->sel());

    auto cap = current->pd->Space_obj::lookup (r->pd());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_pd::PD))) {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto pd = new Pd (Pd::current, r->sel(), cap.prm());
    if (current->pd->Space_obj::insert (r->sel(), Capability (pd, cap.prm())) != Status::SUCCESS) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
        delete pd;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_create_ec()
{
    Sys_create_ec *r = static_cast<Sys_create_ec *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE EC:%#lx CPU:%#x UTCB:%#lx ESP:%#lx EVT:%#x", current, r->sel(), r->cpu(), r->utcb(), r->esp(), r->evt());

    if (EXPECT_FALSE (!Hip::hip->cpu_online (r->cpu()))) {
        trace (TRACE_ERROR, "%s: Invalid CPU (%#x)", __func__, r->cpu());
        sys_finish<Sys_regs::BAD_CPU>();
    }

    if (EXPECT_FALSE (!r->utcb() && !(Hip::hip->feature() & (Hip::FEAT_VMX | Hip::FEAT_SVM)))) {
        trace (TRACE_ERROR, "%s: VCPUs not supported", __func__);
        sys_finish<Sys_regs::BAD_FTR>();
    }

    auto cap = current->pd->Space_obj::lookup (r->pd());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_pd::EC_PT_SM))) {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Sys_regs::BAD_CAP>();
    }
    auto pd = static_cast<Pd *>(cap.obj());

    if (EXPECT_FALSE (r->utcb() >= USER_ADDR || r->utcb() & PAGE_MASK || !pd->insert_utcb (r->utcb()))) {
        trace (TRACE_ERROR, "%s: Invalid UTCB address (%#lx)", __func__, r->utcb());
        sys_finish<Sys_regs::BAD_PAR>();
    }

    auto ec = new Ec (Pd::current, r->sel(), pd, r->flags() & 1 ? static_cast<void (*)()>(send_msg<ret_user_iret>) : nullptr, r->cpu(), r->evt(), r->utcb(), r->esp());

    if (current->pd->Space_obj::insert (r->sel(), Capability (ec, static_cast<unsigned>(Capability::Perm_ec::DEFINED))) != Status::SUCCESS) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
        delete ec;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_create_sc()
{
    Sys_create_sc *r = static_cast<Sys_create_sc *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE SC:%#lx EC:%#lx P:%#x Q:%#x", current, r->sel(), r->ec(), r->qpd().prio(), r->qpd().quantum());

    auto cap = current->pd->Space_obj::lookup (r->pd());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_pd::SC))) {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    cap = current->pd->Space_obj::lookup (r->ec());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_ec::BIND_SC))) {
        trace (TRACE_ERROR, "%s: Non-EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto ec = static_cast<Ec *>(cap.obj());

    if (EXPECT_FALSE (!ec->glb)) {
        trace (TRACE_ERROR, "%s: Cannot bind SC", __func__);
        sys_finish<Sys_regs::BAD_CAP>();
    }

    if (EXPECT_FALSE (!r->qpd().prio() || !r->qpd().quantum())) {
        trace (TRACE_ERROR, "%s: Invalid QPD", __func__);
        sys_finish<Sys_regs::BAD_PAR>();
    }

    auto sc = new Sc (Pd::current, r->sel(), ec, ec->cpu, r->qpd().prio(), r->qpd().quantum());
    if (current->pd->Space_obj::insert (r->sel(), Capability (sc, static_cast<unsigned>(Capability::Perm_sc::DEFINED))) != Status::SUCCESS) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
        delete sc;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    sc->remote_enqueue();

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_create_pt()
{
    Sys_create_pt *r = static_cast<Sys_create_pt *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE PT:%#lx EC:%#lx EIP:%#lx", current, r->sel(), r->ec(), r->eip());

    auto cap = current->pd->Space_obj::lookup (r->pd());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_pd::EC_PT_SM))) {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    cap = current->pd->Space_obj::lookup (r->ec());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_ec::BIND_PT))) {
        trace (TRACE_ERROR, "%s: Non-EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto ec = static_cast<Ec *>(cap.obj());

    if (EXPECT_FALSE (ec->glb)) {
        trace (TRACE_ERROR, "%s: Cannot bind PT", __func__);
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto pt = new Pt (Pd::current, r->sel(), ec, r->mtd(), r->eip());
    if (current->pd->Space_obj::insert (r->sel(), Capability (pt, static_cast<unsigned>(Capability::Perm_pt::DEFINED))) != Status::SUCCESS) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
        delete pt;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_create_sm()
{
    Sys_create_sm *r = static_cast<Sys_create_sm *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE SM:%#lx CNT:%lu", current, r->sel(), r->cnt());

    auto cap = current->pd->Space_obj::lookup (r->pd());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_pd::EC_PT_SM))) {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto sm = new Sm (Pd::current, r->sel(), r->cnt());
    if (current->pd->Space_obj::insert (r->sel(), Capability (sm, static_cast<unsigned>(Capability::Perm_sm::DEFINED))) != Status::SUCCESS) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
        delete sm;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_ec_ctrl()
{
    Sys_ec_ctrl *r = static_cast<Sys_ec_ctrl *>(current->sys_regs());

    auto cap = current->pd->Space_obj::lookup (r->ec());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_ec::CTRL))) {
        trace (TRACE_ERROR, "%s: Bad EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto ec = static_cast<Ec *>(cap.obj());

    if (!(ec->hazard & HZD_RECALL)) {

        ec->set_hazard (HZD_RECALL);

        if (Cpu::id != ec->cpu && Ec::remote (ec->cpu) == ec)
            Interrupt::send_cpu (ec->cpu, Interrupt::Request::RKE);
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_sc_ctrl()
{
    Sys_sc_ctrl *r = static_cast<Sys_sc_ctrl *>(current->sys_regs());

    auto cap = current->pd->Space_obj::lookup (r->sc());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_sc::CTRL))) {
        trace (TRACE_ERROR, "%s: Bad SC CAP (%#lx)", __func__, r->sc());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    r->set_time (static_cast<Sc *>(cap.obj())->time * 1000 / Lapic::freq_tsc);

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_pt_ctrl()
{
    Sys_pt_ctrl *r = static_cast<Sys_pt_ctrl *>(current->sys_regs());

    auto cap = current->pd->Space_obj::lookup (r->pt());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_pt::CTRL))) {
        trace (TRACE_ERROR, "%s: Bad PT CAP (%#lx)", __func__, r->pt());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto pt = static_cast<Pt *>(cap.obj());

    pt->set_id (r->id());

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_sm_ctrl()
{
    Sys_sm_ctrl *r = static_cast<Sys_sm_ctrl *>(current->sys_regs());

    auto cap = current->pd->Space_obj::lookup (r->sm());
    if (EXPECT_FALSE (!cap.validate (r->op() ? Capability::Perm_sm::CTRL_DN : Capability::Perm_sm::CTRL_UP))) {
        trace (TRACE_ERROR, "%s: Bad SM CAP (%#lx)", __func__, r->sm());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto sm = static_cast<Sm *>(cap.obj());

    switch (r->op()) {

        case 0:
            sm->up();
            break;

        case 1:
#if 0       // FIXME
            if (sm->space == static_cast<Space_obj *>(&Pd::kern))
                Gsi::unmask (static_cast<unsigned>(sm->node_base - NUM_CPU));
#endif
            sm->dn (r->zc(), r->time());
            break;
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_assign_gsi()
{
    Sys_assign_gsi *r = static_cast<Sys_assign_gsi *>(current->sys_regs());

    if (EXPECT_FALSE (!Hip::hip->cpu_online (r->cpu()))) {
        trace (TRACE_ERROR, "%s: Invalid CPU (%#x)", __func__, r->cpu());
        sys_finish<Sys_regs::BAD_CPU>();
    }

    auto cap = current->pd->Space_obj::lookup (r->sm());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_sm::ASSIGN))) {
        trace (TRACE_ERROR, "%s: Non-SM CAP (%#lx)", __func__, r->sm());
        sys_finish<Sys_regs::BAD_CAP>();
    }

#if 0       // FIXME
    auto sm = static_cast<Sm *>(cap.obj());

    if (EXPECT_FALSE (sm->space != static_cast<Space_obj *>(&Pd::kern))) {
        trace (TRACE_ERROR, "%s: Non-GSI SM (%#lx)", __func__, r->sm());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    uint64 phys; unsigned o, rid = 0, gsi = static_cast<unsigned>(sm->node_base - NUM_CPU);
    if (EXPECT_FALSE (!Gsi::gsi_table[gsi].ioapic && (!Pd::current->Space_mem::lookup (r->dev(), phys, o) || ((rid = Pci::phys_to_rid (phys)) == ~0U && (rid = Hpet::phys_to_rid (phys)) == ~0U)))) {
        trace (TRACE_ERROR, "%s: Non-DEV CAP (%#lx)", __func__, r->dev());
        sys_finish<Sys_regs::BAD_DEV>();
    }

    r->set_msi (Gsi::set (gsi, r->cpu(), rid));
#endif

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_assign_dev()
{
    auto r = static_cast<Sys_assign_dev *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s PD:%#lx SMMU:%#lx SI:%u DEV:%#lx", static_cast<void *>(current), __func__, r->pd(), r->smmu(), static_cast<unsigned>(r->si()), r->dev());

    if (EXPECT_FALSE (current->pd != &Pd::root)) {
        trace (TRACE_ERROR, "%s: Not Root PD", __func__);
        sys_finish<Sys_regs::BAD_HYP>();
    }

    Smmu *smmu = Smmu::lookup (r->smmu());

    if (EXPECT_FALSE (!smmu)) {
        trace (TRACE_ERROR, "%s: Bad SMMU (%#lx)", __func__, r->smmu());
        sys_finish<Sys_regs::BAD_DEV>();
    }

    auto cap = current->pd->Space_obj::lookup (r->pd());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_pd::ASSIGN))) {
        trace (TRACE_ERROR, "%s: Bad PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    auto pd = static_cast<Pd *>(cap.obj());

    if (!smmu->configure (pd, r->si(), r->dev())) {
        trace (TRACE_ERROR, "%s: Bad Parameter for SI/DEV", __func__);
        sys_finish<Sys_regs::BAD_PAR>();
    }

    sys_finish<Sys_regs::SUCCESS>();
}

extern "C"
void (*const syscall[16])() =
{
    &Ec::sys_call,
    &Ec::sys_reply,
    &Ec::sys_create_pd,
    &Ec::sys_create_ec,
    &Ec::sys_create_sc,
    &Ec::sys_create_pt,
    &Ec::sys_create_sm,
    &Ec::sys_finish<Sys_regs::BAD_HYP>,
    &Ec::sys_ec_ctrl,
    &Ec::sys_sc_ctrl,
    &Ec::sys_pt_ctrl,
    &Ec::sys_sm_ctrl,
    &Ec::sys_finish<Sys_regs::BAD_HYP>,
    &Ec::sys_assign_gsi,
    &Ec::sys_assign_dev,
    &Ec::sys_finish<Sys_regs::BAD_HYP>,
};

template void Ec::sys_finish<Sys_regs::COM_ABT>();
template void Ec::send_msg<Ec::ret_user_vmresume>();
template void Ec::send_msg<Ec::ret_user_vmrun>();
template void Ec::send_msg<Ec::ret_user_iret>();
