/*
 * System-Call Interface
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "dmar.h"
#include "gsi.h"
#include "hip.h"
#include "lapic.h"
#include "pci.h"
#include "pt.h"
#include "sm.h"
#include "syscall.h"
#include "utcb.h"

template <Sys_regs::Status T>
void Ec::sys_finish()
{
    current->regs.set_status (T);
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

template <bool C>
void Ec::delegate()
{
    Ec *ec = current->rcap;
    assert (ec);

    Ec *src = C ? ec : current;
    Ec *dst = C ? current : ec;

    bool user = C || dst->cont == ret_user_sysexit;

    dst->pd->xfer_items (src->pd,
                         user ? dst->utcb->xlt : Crd (0),
                         user ? dst->utcb->del : Crd (Crd::MEM, 0, Crd::whole, 0),
                         src->utcb->xfer(),
                         user ? dst->utcb->xfer() : 0,
                         src->utcb->ti);

    C ? ret_user_sysexit() : reply();
}

template <void (*C)()>
void Ec::send_msg()
{
    Exc_regs *r = &current->regs;

    Kobject *obj = Space_obj::lookup (current->evt + r->dst_portal).obj();
    if (EXPECT_FALSE (obj->type() != Kobject::PT))
        die ("PT not found");

    Pt *pt = static_cast<Pt *>(obj);
    Ec *ec = pt->ec;

    if (EXPECT_FALSE (current->cpu != ec->xcpu))
        die ("PT wrong CPU");

    if (EXPECT_TRUE (!ec->cont)) {
        current->cont = C;
        current->set_partner (ec);
        current->regs.mtd = pt->mtd.val;
        ec->cont = recv_kern;
        ec->regs.set_pt (pt->node_base);
        ec->regs.set_ip (pt->ip);
        ec->make_current();
    }

    ec->help (send_msg<C>);

    die ("IPC Timeout");
}

void Ec::sys_call()
{
    Sys_call *s = static_cast<Sys_call *>(current->sys_regs());

    Kobject *obj = Space_obj::lookup (s->pt()).obj();
    if (EXPECT_FALSE (obj->type() != Kobject::PT))
        sys_finish<Sys_regs::BAD_CAP>();

    Pt *pt = static_cast<Pt *>(obj);
    Ec *ec = pt->ec;

    if (EXPECT_FALSE (current->cpu != ec->xcpu))
        sys_finish<Sys_regs::BAD_CPU>();

    if (EXPECT_TRUE (!ec->cont)) {
        current->cont = ret_user_sysexit;
        current->set_partner (ec);
        ec->cont = recv_user;
        ec->regs.set_ip (pt->ip);
        ec->regs.set_pt (pt->node_base);
        ec->make_current();
    }

    if (EXPECT_TRUE (!(s->flags() & Sys_call::DISABLE_BLOCKING)))
        ec->help (sys_call);

    sys_finish<Sys_regs::IPC_TIM>();
}

void Ec::recv_kern()
{
    Ec *ec = current->rcap;

    if (ec->cont == ret_user_iret)
        current->utcb->load_exc (&ec->regs);
    else if (ec->cont == ret_user_vmresume)
        current->utcb->load_vmx (&ec->regs);
    else if (ec->cont == ret_user_vmrun)
        current->utcb->load_svm (&ec->regs);

    ret_user_sysexit();
}

void Ec::recv_user()
{
    Ec *ec = current->rcap;

    ec->utcb->save (current->utcb);

    if (EXPECT_FALSE (ec->utcb->ti))
        delegate<true>();

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

        if (EXPECT_TRUE (ec->cont == ret_user_sysexit))
            src->save (ec->utcb);
        else if (ec->cont == ret_user_iret)
            src->save_exc (&ec->regs);
        else if (ec->cont == ret_user_vmresume)
            src->save_vmx (&ec->regs);
        else if (ec->cont == ret_user_vmrun)
            src->save_svm (&ec->regs);

        if (EXPECT_FALSE (src->ti))
            delegate<false>();
    }

    reply();
}

void Ec::sys_create_pd()
{
    Sys_create_pd *r = static_cast<Sys_create_pd *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE PD:%#lx", current, r->sel());

    Capability cap = Space_obj::lookup (r->pd());
    if (EXPECT_FALSE (cap.obj()->type() != Kobject::PD) || !(cap.prm() & 1UL << Kobject::PD)) {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Sys_regs::BAD_CAP>();
    }
    Pd *dst = static_cast<Pd *>(cap.obj());

    Pd *pd = new Pd (dst, r->sel(), cap.prm());
    if (!dst->Space_obj::insert_root (pd)) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
        delete pd;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    Crd crd = r->crd();
    pd->del_crd (dst, Crd (Crd::OBJ, 0, Crd::whole, 0), crd);

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_create_ec()
{
    Sys_create_ec *r = static_cast<Sys_create_ec *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE EC:%#lx CPU:%#x UTCB:%#lx ESP:%#lx EVT:%#x", current, r->sel(), r->cpu(), r->utcb(), r->esp(), r->evt());

    if (EXPECT_FALSE (!Hip::cpu_online (r->cpu()))) {
        trace (TRACE_ERROR, "%s: Invalid CPU (%#x)", __func__, r->cpu());
        sys_finish<Sys_regs::BAD_CPU>();
    }

    if (EXPECT_FALSE (!r->utcb() && !(Hip::feature() & (Hip::FEAT_VMX | Hip::FEAT_SVM)))) {
        trace (TRACE_ERROR, "%s: VCPUs not supported", __func__);
        sys_finish<Sys_regs::BAD_FTR>();
    }

    Capability cap = Space_obj::lookup (r->pd());
    if (EXPECT_FALSE (cap.obj()->type() != Kobject::PD) || !(cap.prm() & 1UL << Kobject::EC)) {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Sys_regs::BAD_CAP>();
    }
    Pd *dst = static_cast<Pd *>(cap.obj());

    if (EXPECT_FALSE (r->utcb() >= USER_ADDR || r->utcb() & PAGE_MASK || !dst->insert_utcb (r->utcb()))) {
        trace (TRACE_ERROR, "%s: Invalid UTCB address (%#lx)", __func__, r->utcb());
        sys_finish<Sys_regs::BAD_MEM>();
    }

    Ec *ec = new Ec (dst, r->sel(), r->flags() & 1 ? send_msg<ret_user_iret> : 0, r->cpu(), r->evt(), r->utcb(), r->esp());

    if (!dst->Space_obj::insert_root (ec)) {
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

    Capability cap = Space_obj::lookup (r->pd());
    if (EXPECT_FALSE (cap.obj()->type() != Kobject::PD) || !(cap.prm() & 1UL << Kobject::SC)) {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Sys_regs::BAD_CAP>();
    }
    Pd *dst = static_cast<Pd *>(cap.obj());

    Kobject *e;
    if (!dst->Space_obj::lookup (r->ec(), cap) || (e = cap.obj())->type() != Kobject::EC) {
        trace (TRACE_ERROR, "%s: Non-EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Sys_regs::BAD_CAP>();
    }
    Ec *ec = static_cast<Ec *>(e);

    if (EXPECT_FALSE (!ec->glb || !(cap.prm() & 1UL << Kobject::SC))) {
        trace (TRACE_ERROR, "%s: Cannot bind SC", __func__);
        sys_finish<Sys_regs::BAD_CAP>();
    }

    Sc *sc = new Sc (dst, r->sel(), ec, ec->cpu, r->qpd().prio(), r->qpd().quantum());
    if (!dst->Space_obj::insert_root (sc)) {
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

    Capability cap = Space_obj::lookup (r->pd());
    if (EXPECT_FALSE (cap.obj()->type() != Kobject::PD) || !(cap.prm() & 1UL << Kobject::PT)) {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Sys_regs::BAD_CAP>();
    }
    Pd *dst = static_cast<Pd *>(cap.obj());

    Kobject *e;
    if (!dst->Space_obj::lookup (r->ec(), cap) || (e = cap.obj())->type() != Kobject::EC) {
        trace (TRACE_ERROR, "%s: Non-EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Sys_regs::BAD_CAP>();
    }
    Ec *ec = static_cast<Ec *>(e);

    if (EXPECT_FALSE (ec->glb || ec->pd != dst || !(cap.prm() & 1UL << Kobject::PT))) {
        trace (TRACE_ERROR, "%s: Cannot bind PT", __func__);
        sys_finish<Sys_regs::BAD_CAP>();
    }

    Pt *pt = new Pt (dst, r->sel(), ec, r->mtd(), r->eip());
    if (!dst->Space_obj::insert_root (pt)) {
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

    Capability cap = Space_obj::lookup (r->pd());
    if (EXPECT_FALSE (cap.obj()->type() != Kobject::PD) || !(cap.prm() & 1UL << Kobject::SM)) {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Sys_regs::BAD_CAP>();
    }
    Pd *dst = static_cast<Pd *>(cap.obj());

    Sm *sm = new Sm (dst, r->sel(), r->cnt());
    if (!dst->Space_obj::insert_root (sm)) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
        delete sm;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_revoke()
{
    Sys_revoke *r = static_cast<Sys_revoke *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_REVOKE", current);

    Pd::current->rev_crd (r->crd(), r->flags());

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_lookup()
{
    Sys_lookup *s = static_cast<Sys_lookup *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_LOOKUP T:%#x B:%#lx", current, s->crd().type(), s->crd().base());

    Mdb *mdb = Pd::current->lookup_crd (s->crd());

    s->crd() = mdb ? Crd (s->crd().type(), mdb->node_base, mdb->node_order, mdb->node_attr) : Crd (0);

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_ec_ctrl()
{
    Sys_ec_ctrl *r = static_cast<Sys_ec_ctrl *>(current->sys_regs());

    Capability cap = Space_obj::lookup (r->ec());
    if (EXPECT_FALSE (cap.obj()->type() != Kobject::EC || !(cap.prm() & 1UL << 0))) {
        trace (TRACE_ERROR, "%s: Non-EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    Ec *ec = static_cast<Ec *>(cap.obj());

    if (!(ec->regs.hazard() & HZD_RECALL)) {

        ec->regs.set_hazard (HZD_RECALL);

        if (Cpu::id != ec->cpu && Ec::remote (ec->cpu) == ec)
            Lapic::send_ipi (ec->cpu, Lapic::DLV_FIXED, VEC_IPI_RKE);
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_sc_ctrl()
{
    Sys_sc_ctrl *r = static_cast<Sys_sc_ctrl *>(current->sys_regs());

    Capability cap = Space_obj::lookup (r->sc());
    if (EXPECT_FALSE (cap.obj()->type() != Kobject::SC || !(cap.prm() & 1UL << 0))) {
        trace (TRACE_ERROR, "%s: Non-SC CAP (%#lx)", __func__, r->sc());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    r->set_time (static_cast<Sc *>(cap.obj())->time);

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_sm_ctrl()
{
    Sys_sm_ctrl *r = static_cast<Sys_sm_ctrl *>(current->sys_regs());

    Capability cap = Space_obj::lookup (r->sm());
    if (EXPECT_FALSE (cap.obj()->type() != Kobject::SM || !(cap.prm() & 1UL << r->op()))) {
        trace (TRACE_ERROR, "%s: Non-SM CAP (%#lx)", __func__, r->sm());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    Sm *sm = static_cast<Sm *>(cap.obj());

    switch (r->op()) {

        case 0:
            sm->up();
            break;

        case 1:
            if (sm->node_pd == &Pd::kern)
                Gsi::unmask (static_cast<unsigned>(sm->node_base));
            current->cont = sys_finish<Sys_regs::SUCCESS>;
            sm->dn (r->zc());
            break;
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_assign_pci()
{
    Sys_assign_pci *r = static_cast<Sys_assign_pci *>(current->sys_regs());

    Kobject *obj = Space_obj::lookup (r->pd()).obj();
    if (EXPECT_FALSE (obj->type() != Kobject::PD)) {
        trace (TRACE_ERROR, "%s: Non-PD CAP (%#lx)", __func__, r->pd());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    Dmar *dmar = Pci::find_dmar (r->pf());
    if (EXPECT_FALSE (!dmar)) {
        trace (TRACE_ERROR, "%s: RID not found (%#lx)", __func__, r->pf());
        sys_finish<Sys_regs::BAD_DEV>();
    }

    dmar->assign (r->vf() ? r->vf() : r->pf(), static_cast<Pd *>(obj));

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_assign_gsi()
{
    Sys_assign_gsi *r = static_cast<Sys_assign_gsi *>(current->sys_regs());

    if (EXPECT_FALSE (!Hip::cpu_online (r->cpu()))) {
        trace (TRACE_ERROR, "%s: Invalid CPU (%#x)", __func__, r->cpu());
        sys_finish<Sys_regs::BAD_CPU>();
    }

    Kobject *obj = Space_obj::lookup (r->sm()).obj();
    if (EXPECT_FALSE (obj->type() != Kobject::SM)) {
        trace (TRACE_ERROR, "%s: Non-SM CAP (%#lx)", __func__, r->sm());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    Sm *sm = static_cast<Sm *>(obj);

    if (EXPECT_FALSE (sm->node_pd != &Pd::kern)) {
        trace (TRACE_ERROR, "%s: Non-VEC SM (%#lx)", __func__, r->sm());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    r->set_msi (Gsi::set (static_cast<unsigned>(sm->node_base), r->cpu(), r->rid()));

    sys_finish<Sys_regs::SUCCESS>();
}

extern "C"
void (*const syscall[])() =
{
    &Ec::sys_call,
    &Ec::sys_reply,
    &Ec::sys_create_pd,
    &Ec::sys_create_ec,
    &Ec::sys_create_sc,
    &Ec::sys_create_pt,
    &Ec::sys_create_sm,
    &Ec::sys_revoke,
    &Ec::sys_lookup,
    &Ec::sys_ec_ctrl,
    &Ec::sys_sc_ctrl,
    &Ec::sys_sm_ctrl,
    &Ec::sys_assign_pci,
    &Ec::sys_assign_gsi,
    &Ec::sys_finish<Sys_regs::BAD_HYP>,
    &Ec::sys_finish<Sys_regs::BAD_HYP>,
    &Ec::sys_finish<Sys_regs::BAD_HYP>,
};

template void Ec::sys_finish<Sys_regs::IPC_ABT>();
template void Ec::send_msg<Ec::ret_user_vmresume>();
template void Ec::send_msg<Ec::ret_user_vmrun>();
template void Ec::send_msg<Ec::ret_user_iret>();
