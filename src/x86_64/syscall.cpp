/*
 * System-Call Interface
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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
#include "hip.hpp"
#include "interrupt.hpp"
#include "lapic.hpp"
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
#include "utcb.hpp"
#include "vectors.hpp"

void Ec::sys_finish_status (Status s)
{
    Sys_abi (sys_regs()).p0() = std::to_underlying (s);

    ret_user_sysexit();
}

template <Status S, bool T>
void Ec::sys_finish()
{
    if (T)
        current->clr_timeout();

    current->sys_finish_status (S);
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
    auto r = current->exc_regs();

    auto cap = current->get_obj()->lookup (current->evt + r.ep());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_pt::EVENT)))
        die ("PT not found");

    auto pt = static_cast<Pt *>(cap.obj());
    auto ec = pt->ec;

    if (EXPECT_FALSE (current->cpu != ec->xcpu))
        die ("PT wrong CPU");

    if (EXPECT_TRUE (!ec->cont)) {
        current->cont = C;
        current->set_partner (ec);
        ec->cont = recv_kern;
        ec->exc_regs().ip() = pt->ip;
        auto abi = Sys_abi (ec->sys_regs());
        abi.p0() = pt->id;
        abi.p1() = pt->mtd;
        ec->make_current();
    }

    ec->help (send_msg<C>);

    die ("IPC Timeout");
}

void Ec::sys_call()
{
    auto r = Sys_ipc_call (current->sys_regs());

    auto cap = current->get_obj()->lookup (r.pt());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_pt::CALL)))
        sys_finish<Status::BAD_CAP>();

    auto pt = static_cast<Pt *>(cap.obj());
    auto ec = pt->ec;

    if (EXPECT_FALSE (current->cpu != ec->xcpu))
        sys_finish<Status::BAD_CPU>();

    if (EXPECT_TRUE (!ec->cont)) {
        current->cont = ret_user_sysexit;
        current->set_partner (ec);
        ec->cont = recv_user;
        ec->exc_regs().ip() = pt->ip;
        auto abi = Sys_abi (ec->sys_regs());
        abi.p0() = pt->id;
        abi.p1() = r.mtd();
        ec->make_current();
    }

    if (EXPECT_TRUE (!r.timeout()))
        ec->help (sys_call);

    sys_finish<Status::TIMEOUT>();
}

void Ec::recv_kern()
{
    Ec *ec = current->rcap;

    Mtd_arch mtd = Sys_ipc_reply (current->sys_regs()).mtd_a();

    if (ec->cont == ret_user_iret)
        current->utcb->arch()->load_exc (mtd, ec->exc_regs());
    else if (ec->cont == ret_user_vmresume)
        current->utcb->arch()->load_vmx (mtd, ec->cpu_regs());
    else if (ec->cont == ret_user_vmrun)
        current->utcb->arch()->load_svm (mtd, ec->cpu_regs());

    ret_user_sysexit();
}

void Ec::recv_user()
{
    Ec *ec = current->rcap;

    Mtd_user mtd = Sys_ipc_reply (current->sys_regs()).mtd_u();

    ec->utcb->copy (mtd, current->utcb);

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
    auto r = Sys_ipc_reply (current->sys_regs());

    Ec *ec = current->rcap;

    if (EXPECT_TRUE (ec)) {

        Utcb *src = current->utcb;

        if (EXPECT_TRUE (ec->cont == ret_user_sysexit)) {
            Sys_abi (ec->sys_regs()).p1() = r.mtd_u();
            src->copy (r.mtd_u(), ec->utcb);
        }
        else if (ec->cont == ret_user_iret)
            src->arch()->save_exc (r.mtd_a(), ec->exc_regs());
        else if (ec->cont == ret_user_vmresume)
            src->arch()->save_vmx (r.mtd_a(), ec->cpu_regs(), current->get_obj());
        else if (ec->cont == ret_user_vmrun)
            src->arch()->save_svm (r.mtd_a(), ec->cpu_regs(), current->get_obj());
    }

    reply();
}

void Ec::sys_create_pd()
{
    auto r { Sys_create_pd (current->sys_regs()) };

    trace (TRACE_SYSCALL, "EC:%p %s SEL:%#lx PD:%#lx (OP:%u)", static_cast<void *>(current), __func__, r.sel(), r.pd(), r.op());

    auto const cpd { current->get_obj()->lookup (r.pd()) };

    if (EXPECT_FALSE (!cpd.validate (Capability::Perm_pd::PD)))
        current->sys_finish_status (Status::BAD_CAP);

    auto const pd { static_cast<Pd *>(cpd.obj()) };

    Status s;

    switch (static_cast<Kobject::Subtype>(r.op())) {
        default: s = Status::BAD_PAR; break;
        case Kobject::Subtype::PD:  Pd::create_pd  (s, current->get_obj(), r.sel(), cpd.prm()); break;
        case Kobject::Subtype::OBJ: pd->create_obj (s, current->get_obj(), r.sel()); break;
        case Kobject::Subtype::HST: pd->create_hst (s, current->get_obj(), r.sel()); break;
        case Kobject::Subtype::GST: pd->create_gst (s, current->get_obj(), r.sel()); break;
        case Kobject::Subtype::DMA: pd->create_dma (s, current->get_obj(), r.sel()); break;
        case Kobject::Subtype::PIO: pd->create_pio (s, current->get_obj(), r.sel()); break;
        case Kobject::Subtype::MSR: pd->create_msr (s, current->get_obj(), r.sel()); break;
    }

    current->sys_finish_status (s);
}

void Ec::sys_create_ec()
{
    auto r = Sys_create_ec (current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s EC:%#lx CPU:%#x UTCB:%#lx ESP:%#lx EVT:%#x", current, __func__, r.sel(), r.cpu(), r.utcb(), r.esp(), r.evt());

    if (EXPECT_FALSE (!Hip::hip->cpu_online (r.cpu()))) {
        trace (TRACE_ERROR, "%s: Invalid CPU (%#x)", __func__, r.cpu());
        sys_finish<Status::BAD_CPU>();
    }

    if (EXPECT_FALSE (r.utcb() >= Space_hst::num << PAGE_BITS)) {
        trace (TRACE_ERROR, "%s: Invalid UTCB address (%#lx)", __func__, r.utcb());
        sys_finish<Status::BAD_PAR>();
    }

    if (EXPECT_FALSE (!r.utcb() && !(Hip::hip->feature() & (Hip::FEAT_VMX | Hip::FEAT_SVM)))) {
        trace (TRACE_ERROR, "%s: VCPUs not supported", __func__);
        sys_finish<Status::BAD_FTR>();
    }

    auto cap = current->get_obj()->lookup (r.pd());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_pd::EC))) {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r.pd());
        sys_finish<Status::BAD_CAP>();
    }
    auto pd = static_cast<Pd *>(cap.obj());

    auto ec = new Ec (pd->get_obj(), pd->get_hst(), pd->get_pio(), r.sel(), r.type() ? static_cast<void (*)()>(send_msg<ret_user_iret>) : nullptr, r.cpu(), r.evt(), r.utcb(), r.esp());

    if (current->get_obj()->insert (r.sel(), Capability (ec, static_cast<unsigned>(Capability::Perm_ec::DEFINED))) != Status::SUCCESS) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r.sel());
        delete ec;
        sys_finish<Status::BAD_CAP>();
    }

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_create_sc()
{
    auto r = Sys_create_sc (current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s SC:%#lx EC:%#lx P:%#x Q:%#x", current, __func__, r.sel(), r.ec(), r.qpd().prio(), r.qpd().quantum());

    auto cpd { current->get_obj()->lookup (r.pd()) };
    if (EXPECT_FALSE (!cpd.validate (Capability::Perm_pd::SC))) {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r.pd());
        sys_finish<Status::BAD_CAP>();
    }

    auto cec { current->get_obj()->lookup (r.ec()) };
    if (EXPECT_FALSE (!cec.validate (Capability::Perm_ec::BIND_SC))) {
        trace (TRACE_ERROR, "%s: Non-EC CAP (%#lx)", __func__, r.ec());
        sys_finish<Status::BAD_CAP>();
    }

    auto ec = static_cast<Ec *>(cec.obj());

    if (EXPECT_FALSE (!ec->glb)) {
        trace (TRACE_ERROR, "%s: Cannot bind SC", __func__);
        sys_finish<Status::BAD_CAP>();
    }

    if (EXPECT_FALSE (!r.qpd().prio() || !r.qpd().quantum())) {
        trace (TRACE_ERROR, "%s: Invalid QPD", __func__);
        sys_finish<Status::BAD_PAR>();
    }

    auto sc = new Sc (current->pd, r.sel(), ec, ec->cpu, r.qpd().prio(), r.qpd().quantum());
    if (current->get_obj()->insert (r.sel(), Capability (sc, static_cast<unsigned>(Capability::Perm_sc::DEFINED))) != Status::SUCCESS) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r.sel());
        delete sc;
        sys_finish<Status::BAD_CAP>();
    }

    sc->remote_enqueue();

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_create_pt()
{
    auto r = Sys_create_pt (current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s PT:%#lx EC:%#lx EIP:%#lx", current, __func__, r.sel(), r.ec(), r.eip());

    auto cpd { current->get_obj()->lookup (r.pd()) };
    if (EXPECT_FALSE (!cpd.validate (Capability::Perm_pd::PT))) {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r.pd());
        sys_finish<Status::BAD_CAP>();
    }

    auto cec { current->get_obj()->lookup (r.ec()) };
    if (EXPECT_FALSE (!cec.validate (Capability::Perm_ec::BIND_PT))) {
        trace (TRACE_ERROR, "%s: Non-EC CAP (%#lx)", __func__, r.ec());
        sys_finish<Status::BAD_CAP>();
    }

    auto ec = static_cast<Ec *>(cec.obj());

    if (EXPECT_FALSE (ec->glb)) {
        trace (TRACE_ERROR, "%s: Cannot bind PT", __func__);
        sys_finish<Status::BAD_CAP>();
    }

    auto pt = new Pt (current->pd, r.sel(), ec, r.mtd(), r.eip());
    if (current->get_obj()->insert (r.sel(), Capability (pt, static_cast<unsigned>(Capability::Perm_pt::DEFINED))) != Status::SUCCESS) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r.sel());
        delete pt;
        sys_finish<Status::BAD_CAP>();
    }

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_create_sm()
{
    auto r = Sys_create_sm (current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s SM:%#lx CNT:%lu", current, __func__, r.sel(), r.cnt());

    auto cap = current->get_obj()->lookup (r.pd());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_pd::SM))) {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r.pd());
        sys_finish<Status::BAD_CAP>();
    }

    auto sm = new Sm (current->pd, r.sel(), r.cnt());
    if (current->get_obj()->insert (r.sel(), Capability (sm, static_cast<unsigned>(Capability::Perm_sm::DEFINED))) != Status::SUCCESS) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r.sel());
        delete sm;
        sys_finish<Status::BAD_CAP>();
    }

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_ctrl_pd()
{
    auto r { Sys_ctrl_pd (current->sys_regs()) };

    trace (TRACE_SYSCALL, "EC:%p %s SRC:%#lx DST:%#lx SSB:%#lx DSB:%#lx ORD:%u PMM:%#x CA:%u SH:%u", static_cast<void *>(current), __func__, r.src(), r.dst(), r.ssb(), r.dsb(), r.ord(), r.pmm(), std::to_underlying (r.ca()), std::to_underlying (r.sh()));

    if (EXPECT_FALSE ((r.ssb() | r.dsb()) & (BITN (r.ord()) - 1)))
        current->sys_finish_status (Status::BAD_PAR);

    auto const cst { current->get_obj()->lookup (r.src()) };
    auto const cdt { current->get_obj()->lookup (r.dst()) };

    Kobject::Subtype st, dt;

    if (EXPECT_TRUE (Capability::validate_take_grant (cst, cdt, st, dt))) {

        if (st == Kobject::Subtype::HST) {
            if (dt == Kobject::Subtype::HST)
                current->sys_finish_status (static_cast<Space_hst *>(cdt.obj())->delegate (static_cast<Space_hst *>(cst.obj()), r.ssb(), r.dsb(), r.ord(), r.pmm(), r.ca(), r.sh()));
            if (dt == Kobject::Subtype::GST)
                current->sys_finish_status (static_cast<Space_gst *>(cdt.obj())->delegate (static_cast<Space_hst *>(cst.obj()), r.ssb(), r.dsb(), r.ord(), r.pmm(), r.ca(), r.sh()));
            if (dt == Kobject::Subtype::DMA)
                current->sys_finish_status (static_cast<Space_dma *>(cdt.obj())->delegate (static_cast<Space_hst *>(cst.obj()), r.ssb(), r.dsb(), r.ord(), r.pmm(), r.ca(), r.sh()));
        }

        else if (st == Kobject::Subtype::OBJ && dt == st)
            current->sys_finish_status (static_cast<Space_obj *>(cdt.obj())->delegate (static_cast<Space_obj *>(cst.obj()), r.ssb(), r.dsb(), r.ord(), r.pmm()));
        else if (st == Kobject::Subtype::PIO && dt == st)
            current->sys_finish_status (static_cast<Space_pio *>(cdt.obj())->delegate (static_cast<Space_pio *>(cst.obj()), r.ssb(), r.dsb(), r.ord(), r.pmm()));
        else if (st == Kobject::Subtype::MSR && dt == st)
            current->sys_finish_status (static_cast<Space_msr *>(cdt.obj())->delegate (static_cast<Space_msr *>(cst.obj()), r.ssb(), r.dsb(), r.ord(), r.pmm()));
    }

    current->sys_finish_status (Status::BAD_CAP);
}

void Ec::sys_ctrl_ec()
{
    auto r = Sys_ctrl_ec (current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s EC:%#lx", current, __func__, r.ec());

    auto cap = current->get_obj()->lookup (r.ec());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_ec::CTRL))) {
        trace (TRACE_ERROR, "%s: Bad EC CAP (%#lx)", __func__, r.ec());
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

void Ec::sys_ctrl_sc()
{
    auto r = Sys_ctrl_sc (current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s SC:%#lx", current, __func__, r.sc());

    auto cap = current->get_obj()->lookup (r.sc());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_sc::CTRL))) {
        trace (TRACE_ERROR, "%s: Bad SC CAP (%#lx)", __func__, r.sc());
        sys_finish<Status::BAD_CAP>();
    }

    r.set_time (static_cast<Sc *>(cap.obj())->time);

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_ctrl_pt()
{
    auto r = Sys_ctrl_pt (current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s PT:%#lx ID:%#lx", current, __func__, r.pt(), r.id());

    auto cap = current->get_obj()->lookup (r.pt());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_pt::CTRL))) {
        trace (TRACE_ERROR, "%s: Bad PT CAP (%#lx)", __func__, r.pt());
        sys_finish<Status::BAD_CAP>();
    }

    auto pt = static_cast<Pt *>(cap.obj());

    pt->set_id (r.id());

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_ctrl_sm()
{
    auto r = Sys_ctrl_sm (current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s SM:%#lx OP:%u", current, __func__, r.sm(), r.op());

    auto cap = current->get_obj()->lookup (r.sm());
    if (EXPECT_FALSE (!cap.validate (r.op() ? Capability::Perm_sm::CTRL_DN : Capability::Perm_sm::CTRL_UP))) {
        trace (TRACE_ERROR, "%s: Bad SM CAP (%#lx)", __func__, r.sm());
        sys_finish<Status::BAD_CAP>();
    }

    auto sm = static_cast<Sm *>(cap.obj());

    switch (r.op()) {

        case 0:
            sm->up();
            break;

        case 1:
#if 0       // FIXME
            if (sm->space == static_cast<Space_obj *>(&Pd::kern))
                Gsi::unmask (static_cast<unsigned>(sm->node_base - NUM_CPU));
#endif
            sm->dn (r.zc(), r.time());
            break;
    }

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_ctrl_hw()
{
    auto r { Sys_ctrl_hw (current->sys_regs()) };

    trace (TRACE_SYSCALL, "EC:%p %s OP:%u DESC:%#lx", static_cast<void *>(current), __func__, r.op(), r.desc());

    if (EXPECT_FALSE (current->get_obj() != Pd::root->get_obj()))
        current->sys_finish_status (Status::BAD_HYP);

    switch (r.op()) {

        default:            // Invalid Operation
            current->sys_finish_status (Status::BAD_PAR);

        case 0:             // S-State Transition
            Acpi_fixed::Transition t { static_cast<uint16>(r.desc()) };

            if (EXPECT_FALSE (!Acpi_fixed::supported (t)))
                current->sys_finish_status (Status::BAD_FTR);

            if (EXPECT_FALSE (!Acpi::set_transition (t)))
                current->sys_finish_status (Status::ABORTED);

            Interrupt::send_exc (Interrupt::Request::RKE);

            Cpu::hazard |= Hazard::SLEEP;

            current->sys_finish_status (Status::SUCCESS);
    }
}

void Ec::sys_assign_int()
{
    auto r = Sys_assign_int (current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p %s SM:%#lx CPU:%u FLG:%#x", current, __func__, r.sm(), r.cpu(), r.flg());

    if (EXPECT_FALSE (!Hip::hip->cpu_online (r.cpu()))) {
        trace (TRACE_ERROR, "%s: Invalid CPU (%#x)", __func__, r.cpu());
        sys_finish<Status::BAD_CPU>();
    }

    auto cap = current->get_obj()->lookup (r.sm());
    if (EXPECT_FALSE (!cap.validate (Capability::Perm_sm::ASSIGN))) {
        trace (TRACE_ERROR, "%s: Non-SM CAP (%#lx)", __func__, r.sm());
        sys_finish<Status::BAD_CAP>();
    }

#if 0       // FIXME
    auto sm = static_cast<Sm *>(cap.obj());

    if (EXPECT_FALSE (sm->space != static_cast<Space_obj *>(&Pd::kern))) {
        trace (TRACE_ERROR, "%s: Non-GSI SM (%#lx)", __func__, r.sm());
        sys_finish<Status::BAD_CAP>();
    }

    uint64 phys; unsigned o, rid = 0, gsi = static_cast<unsigned>(sm->node_base - NUM_CPU);
    if (EXPECT_FALSE (!Gsi::gsi_table[gsi].ioapic && (!Pd::current->Space_mem::lookup (r.dev(), phys, o) || ((rid = Pci::phys_to_rid (phys)) == ~0U && (rid = Hpet::phys_to_rid (phys)) == ~0U)))) {
        trace (TRACE_ERROR, "%s: Non-DEV CAP (%#lx)", __func__, r.dev());
        sys_finish<Status::BAD_DEV>();
    }

    r.set_msi (Gsi::set (gsi, r.cpu(), rid));
#endif

    sys_finish<Status::SUCCESS>();
}

void Ec::sys_assign_dev()
{
    auto r { Sys_assign_dev (current->sys_regs()) };

    trace (TRACE_SYSCALL, "EC:%p %s DMA:%#lx SMMU:%#lx DAD:%#lx", static_cast<void *>(current), __func__, r.dma(), r.smmu(), r.dad());

    if (EXPECT_FALSE (current->get_obj() != Pd::root->get_obj()))
        current->sys_finish_status (Status::BAD_HYP);

    auto const csp { current->get_obj()->lookup (r.dma()) };

    if (EXPECT_FALSE (!csp.validate (Capability::Perm_sp::ASSIGN, Kobject::Subtype::DMA)))
        current->sys_finish_status (Status::BAD_CAP);

    auto const smmu { Smmu::lookup (r.smmu()) };

    if (EXPECT_FALSE (!smmu))
        current->sys_finish_status (Status::BAD_DEV);

    if (EXPECT_FALSE (!smmu->configure (static_cast<Space_dma *>(csp.obj()), r.dad())))
        current->sys_finish_status (Status::BAD_PAR);

    current->sys_finish_status (Status::SUCCESS);
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
    &Ec::sys_ctrl_ec,
    &Ec::sys_ctrl_sc,
    &Ec::sys_ctrl_pt,
    &Ec::sys_ctrl_sm,
    &Ec::sys_ctrl_hw,
    &Ec::sys_assign_int,
    &Ec::sys_assign_dev,
    &Ec::sys_finish<Status::BAD_HYP>,
};

template void Ec::sys_finish<Status::ABORTED>();
template void Ec::send_msg<Ec::ret_user_vmresume>();
template void Ec::send_msg<Ec::ret_user_vmrun>();
template void Ec::send_msg<Ec::ret_user_iret>();
