/*
 * System-Call Interface
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

template <Status S, bool T>
void Ec::sys_finish()
{
    if (T)
        current->clr_timeout();

    current->sys_regs().ARG_1 = std::to_underlying (S);
    ret_user_sysexit();
}

void Ec::activate()
{
    Ec *ec = this;

    // XXX: Make the loop preemptible
    for (Sc::ctr_link = 0; ec->partner; ec = ec->partner)
        Sc::ctr_link++;

    if (ec->blocked()) [[unlikely]]
        ec->block_sc();

    ec->make_current();
}

template <void (*C)()>
void Ec::send_msg()
{
    auto r = current->exc_regs();

    auto cap = Space_obj::lookup (current->evt + r.ep());
    if (!cap.validate (Capability::Perm_pt::EVENT)) [[unlikely]]
        die ("PT not found");

    auto pt = static_cast<Pt *>(cap.obj());
    auto ec = pt->ec;

    if (current->cpu != ec->xcpu) [[unlikely]]
        die ("PT wrong CPU");

    if (!ec->cont) [[likely]] {
        current->cont = C;
        current->set_partner (ec);
        current->regs.mtd = pt->mtd.val;
        ec->cont = recv_kern;
        ec->sys_regs().set_pt (pt->id);
        ec->sys_regs().set_ip (pt->ip);
        ec->make_current();
    }

    ec->help (send_msg<C>);

    die ("IPC Timeout");
}

void Ec::sys_call()
{
    Sys_call *s = static_cast<Sys_call *>(&current->sys_regs());

    auto cap = Space_obj::lookup (s->pt());
    if (!cap.validate (Capability::Perm_pt::CALL)) [[unlikely]]
        sys_finish<Status::BAD_CAP>();

    auto pt = static_cast<Pt *>(cap.obj());
    auto ec = pt->ec;

    if (current->cpu != ec->xcpu) [[unlikely]]
        sys_finish<Status::BAD_CPU>();

    if (!ec->cont) [[likely]] {
        current->cont = ret_user_sysexit;
        current->set_partner (ec);
        ec->cont = recv_user;
        ec->sys_regs().set_pt (pt->id);
        ec->sys_regs().set_ip (pt->ip);
        ec->make_current();
    }

    if (!(s->flags() & Sys_call::DISABLE_BLOCKING)) [[likely]]
        ec->help (sys_call);

    sys_finish<Status::TIMEOUT>();
}

void Ec::recv_kern()
{
    Ec *ec = current->rcap;

    bool fpu = false;

    if (ec->cont == ret_user_iret)
        fpu = current->utcb->load_exc (ec->cpu_regs());
    else if (ec->cont == ret_user_vmresume)
        fpu = current->utcb->load_vmx (ec->cpu_regs());
    else if (ec->cont == ret_user_vmrun)
        fpu = current->utcb->load_svm (ec->cpu_regs());

    if (fpu) [[unlikely]]
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

    if (current->glb) [[unlikely]]
        Sc::schedule (true);

    Ec *ec = current->rcap;

    if (!ec || !ec->clr_partner()) [[unlikely]]
        Sc::current->ec->activate();

    ec->make_current();
}

void Ec::sys_reply()
{
    Ec *ec = current->rcap;

    if (ec) [[likely]] {

        Utcb *src = current->utcb;

        bool fpu = false;

        if (ec->cont == ret_user_sysexit) [[likely]]
            src->save (ec->utcb);
        else if (ec->cont == ret_user_iret)
            fpu = src->save_exc (ec->cpu_regs());
        else if (ec->cont == ret_user_vmresume)
            fpu = src->save_vmx (ec->cpu_regs());
        else if (ec->cont == ret_user_vmrun)
            fpu = src->save_svm (ec->cpu_regs());

        if (fpu) [[unlikely]]
            current->transfer_fpu (ec);
    }

    reply();
}

void Ec::sys_create_pd()
{
    Sys_create_pd *r = static_cast<Sys_create_pd *>(&current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE PD:%#lx", current, r->sel());

    auto cap = Space_obj::lookup (r->pd());
    if (!cap.validate (Capability::Perm_pd::PD)) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Status::BAD_CAP>();
    }

    auto pd = new Pd (Pd::current, r->sel(), cap.prm());
    if (!Space_obj::insert_root (pd)) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
        pd->destroy();
        sys_finish<Status::BAD_CAP>();
    }

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_create_ec()
{
    Sys_create_ec *r = static_cast<Sys_create_ec *>(&current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE EC:%#lx CPU:%#x UTCB:%#lx ESP:%#lx EVT:%#x", current, r->sel(), r->cpu(), r->utcb(), r->esp(), r->evt());

    if (r->cpu() >= Cpu::count) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Invalid CPU (%#x)", __func__, r->cpu());
        sys_finish<Status::BAD_CPU>();
    }

    if (!r->utcb() && !(Hip::hip->feature() & (Hip::FEAT_VMX | Hip::FEAT_SVM))) [[unlikely]] {
        trace (TRACE_ERROR, "%s: VCPUs not supported", __func__);
        sys_finish<Status::BAD_FTR>();
    }

    auto cap = Space_obj::lookup (r->pd());
    if (!cap.validate (Capability::Perm_pd::EC)) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Status::BAD_CAP>();
    }
    auto pd = static_cast<Pd *>(cap.obj());

    if (r->utcb() >= USER_ADDR || r->utcb() & OFFS_MASK (0) || !pd->insert_utcb (r->utcb())) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Invalid UTCB address (%#lx)", __func__, r->utcb());
        sys_finish<Status::BAD_PAR>();
    }

    auto ec = new Ec (Pd::current, r->sel(), pd, r->flags() & 1 ? static_cast<void (*)()>(send_msg<ret_user_iret>) : nullptr, r->cpu(), r->evt(), r->utcb(), r->esp());

    if (!Space_obj::insert_root (ec)) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
        ec->destroy();
        sys_finish<Status::BAD_CAP>();
    }

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_create_sc()
{
    Sys_create_sc *r = static_cast<Sys_create_sc *>(&current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE SC:%#lx EC:%#lx P:%#x Q:%#x", current, r->sel(), r->ec(), r->qpd().prio(), r->qpd().quantum());

    auto cap = Space_obj::lookup (r->pd());
    if (!cap.validate (Capability::Perm_pd::SC)) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Status::BAD_CAP>();
    }

    cap = Space_obj::lookup (r->ec());
    if (!cap.validate (Capability::Perm_ec::BIND_SC)) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Non-EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Status::BAD_CAP>();
    }

    auto ec = static_cast<Ec *>(cap.obj());

    if (!ec->glb) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Cannot bind SC", __func__);
        sys_finish<Status::BAD_CAP>();
    }

    if (!r->qpd().prio() || !r->qpd().quantum()) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Invalid QPD", __func__);
        sys_finish<Status::BAD_PAR>();
    }

    auto sc = new Sc (Pd::current, r->sel(), ec, ec->cpu, r->qpd().prio(), r->qpd().quantum());
    if (!Space_obj::insert_root (sc)) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
        sc->destroy();
        sys_finish<Status::BAD_CAP>();
    }

    sc->remote_enqueue();

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_create_pt()
{
    Sys_create_pt *r = static_cast<Sys_create_pt *>(&current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE PT:%#lx EC:%#lx EIP:%#lx", current, r->sel(), r->ec(), r->eip());

    auto cap = Space_obj::lookup (r->pd());
    if (!cap.validate (Capability::Perm_pd::PT)) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Status::BAD_CAP>();
    }

    cap = Space_obj::lookup (r->ec());
    if (!cap.validate (Capability::Perm_ec::BIND_PT)) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Non-EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Status::BAD_CAP>();
    }

    auto ec = static_cast<Ec *>(cap.obj());

    if (ec->glb) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Cannot bind PT", __func__);
        sys_finish<Status::BAD_CAP>();
    }

    auto pt = new Pt (Pd::current, r->sel(), ec, r->mtd(), r->eip());
    if (!Space_obj::insert_root (pt)) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
        pt->destroy();
        sys_finish<Status::BAD_CAP>();
    }

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_create_sm()
{
    Sys_create_sm *r = static_cast<Sys_create_sm *>(&current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE SM:%#lx CNT:%lu", current, r->sel(), r->cnt());

    auto cap = Space_obj::lookup (r->pd());
    if (!cap.validate (Capability::Perm_pd::SM)) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Status::BAD_CAP>();
    }

    auto sm = new Sm (Pd::current, r->sel(), r->cnt());
    if (!Space_obj::insert_root (sm)) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
        sm->destroy();
        sys_finish<Status::BAD_CAP>();
    }

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_ec_ctrl()
{
    Sys_ec_ctrl *r = static_cast<Sys_ec_ctrl *>(&current->sys_regs());

    auto cap = Space_obj::lookup (r->ec());
    if (!cap.validate (Capability::Perm_ec::CTRL)) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Bad EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Status::BAD_CAP>();
    }

    auto ec = static_cast<Ec *>(cap.obj());

    if (!(ec->regs.hazard & Hazard::RECALL)) {

        ec->regs.hazard.set (Hazard::RECALL);

        if (Cpu::id != ec->cpu && Ec::remote_current (ec->cpu) == ec)
            Interrupt::send_cpu (Interrupt::Request::RKE, ec->cpu);
    }

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_sc_ctrl()
{
    Sys_sc_ctrl *r = static_cast<Sys_sc_ctrl *>(&current->sys_regs());

    auto cap = Space_obj::lookup (r->sc());
    if (!cap.validate (Capability::Perm_sc::CTRL)) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Bad SC CAP (%#lx)", __func__, r->sc());
        sys_finish<Status::BAD_CAP>();
    }

    r->set_time (static_cast<Sc *>(cap.obj())->time);

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_pt_ctrl()
{
    Sys_pt_ctrl *r = static_cast<Sys_pt_ctrl *>(&current->sys_regs());

    auto cap = Space_obj::lookup (r->pt());
    if (!cap.validate (Capability::Perm_pt::CTRL)) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Bad PT CAP (%#lx)", __func__, r->pt());
        sys_finish<Status::BAD_CAP>();
    }

    auto pt = static_cast<Pt *>(cap.obj());

    pt->set_id (r->id());

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_sm_ctrl()
{
    Sys_sm_ctrl *r = static_cast<Sys_sm_ctrl *>(&current->sys_regs());

    auto cap = Space_obj::lookup (r->sm());
    if (!cap.validate (r->op() ? Capability::Perm_sm::CTRL_DN : Capability::Perm_sm::CTRL_UP)) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Bad SM CAP (%#lx)", __func__, r->sm());
        sys_finish<Status::BAD_CAP>();
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

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_assign_dev()
{
    auto const r { static_cast<Sys_assign_dev *>(&current->sys_regs()) };

    trace (TRACE_SYSCALL, "EC:%p %s PD:%#lx SMMU:%#lx DAD:%#lx", static_cast<void *>(current), __func__, r->pd(), r->smmu(), r->dad());

    if (current->pd != &Pd::root) [[unlikely]]
        sys_finish<Status::BAD_HYP>();

    auto const cap { Space_obj::lookup (r->pd()) };

    if (!cap.validate (Capability::Perm_pd::PD)) [[unlikely]]
        sys_finish<Status::BAD_CAP>();

    auto const smmu { Smmu::lookup_phys (r->smmu()) };

    if (!smmu) [[unlikely]]
        sys_finish<Status::BAD_DEV>();

    if (smmu->assign_dev (static_cast<Pd *>(cap.obj()), r->dad()) != Status::SUCCESS) [[unlikely]]
        sys_finish<Status::BAD_PAR>();

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_assign_int()
{
    auto const r { static_cast<Sys_assign_int *>(&current->sys_regs()) };

    trace (TRACE_SYSCALL, "EC:%p %s SM:%#lx CPU:%u IDX:%#x CFG:%#x", static_cast<void *>(current), __func__, r->sm(), r->cpu(), r->idx(), r->cfg());

    if (current->pd != &Pd::root) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Not Root PD", __func__);
        sys_finish<Status::BAD_HYP>();
    }

    if (r->cpu() >= Cpu::count) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Invalid CPU (%#x)", __func__, r->cpu());
        sys_finish<Status::BAD_CPU>();
    }

    auto cap = Space_obj::lookup (r->sm());
    if (!cap.validate (Capability::Perm_sm::CTRL_UP)) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Non-SM CAP (%#lx)", __func__, r->sm());
        sys_finish<Status::BAD_CAP>();
    }

#if 0       // FIXME
    auto sm = static_cast<Sm *>(cap.obj());

    if (sm->space != static_cast<Space_obj *>(&Pd::kern)) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Non-GSI SM (%#lx)", __func__, r->sm());
        sys_finish<Status::BAD_CAP>();
    }

    uint64 phys; unsigned o, rid = 0, gsi = static_cast<unsigned>(sm->node_base - NUM_CPU);
    if (!Gsi::gsi_table[gsi].ioapic && (!Pd::current->Space_mem::lookup (r->dev(), phys, o) || ((rid = Pci::phys_to_rid (phys)) == ~0U && (rid = Hpet::phys_to_rid (phys)) == ~0U))) [[unlikely]] {
        trace (TRACE_ERROR, "%s: Non-DEV CAP (%#lx)", __func__, r->dev());
        sys_finish<Status::BAD_DEV>();
    }

    r->set_msi (Gsi::set (gsi, r->cpu(), rid));
#endif

    sys_finish<Status::SUCCESS>();
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
    &Ec::sys_finish<Status::BAD_HYP>,
    &Ec::sys_ec_ctrl,
    &Ec::sys_sc_ctrl,
    &Ec::sys_pt_ctrl,
    &Ec::sys_sm_ctrl,
    &Ec::sys_finish<Status::BAD_HYP>,
    &Ec::sys_assign_dev,
    &Ec::sys_assign_int,
    &Ec::sys_finish<Status::BAD_HYP>,
};

template void Ec::sys_finish<Status::ABORTED>();
template void Ec::send_msg<Ec::ret_user_vmresume>();
template void Ec::send_msg<Ec::ret_user_vmrun>();
template void Ec::send_msg<Ec::ret_user_iret>();
