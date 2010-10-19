/*
 * System-Call Interface
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
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

void Ec::activate (Ec *ec)
{
    // XXX: Make the loop preemptible
    for (Sc::ctr_link = 0; ec->partner; ec = ec->partner)
        Sc::ctr_link++;

    if (ec->blocked())
        ec->block();

    ec->make_current();
}

template <bool C>
void Ec::delegate()
{
    Ec *ec = current->reply;
    assert (ec);

    Ec *src = C ? ec : current;
    Ec *dst = C ? current : ec;

    bool user = C || dst->cont == ret_user_sysexit;

    dst->pd->xfer_items (src->pd,
                         user ? dst->utcb->crd : Crd (Crd::MEM, 0, Crd::whole, 0),
                         src->utcb->xfer(),
                         user ? dst->utcb->xfer() : 0,
                         src->utcb->ti);

    (C ? ret_user_sysexit : wait_msg)();
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

    if (EXPECT_FALSE (current->cpu != ec->cpu))
        die ("PT wrong CPU");

    if (EXPECT_TRUE (!ec->cont)) {
        current->cont = C;
        current->set_partner (ec);
        current->regs.mtd = pt->mtd.val;
        ec->cont = recv_msg;
        ec->regs.set_pt (pt->node_base);
        ec->regs.set_ip (pt->ip);
        ec->make_current();
    }

    ec->help (send_msg<C>);
}

void Ec::sys_call()
{
    Sys_call *s = static_cast<Sys_call *>(current->sys_regs());

    Kobject *obj = Space_obj::lookup (s->pt()).obj();
    if (EXPECT_FALSE (obj->type() != Kobject::PT))
        sys_finish<Sys_regs::BAD_CAP>();

    Pt *pt = static_cast<Pt *>(obj);
    Ec *ec = pt->ec;

    if (EXPECT_FALSE (current->cpu != ec->cpu))
        sys_finish<Sys_regs::BAD_CPU>();

    if (EXPECT_TRUE (!ec->cont)) {
        current->cont = ret_user_sysexit;
        current->set_partner (ec);
        ec->cont = recv_msg;
        ec->regs.set_ip (pt->ip);
        ec->regs.set_pt (pt->node_base);
        ec->make_current();
    }

    if (EXPECT_FALSE (s->flags() & Sys_call::DISABLE_BLOCKING))
        sys_finish<Sys_regs::TIMEOUT>();

    ec->help (sys_call);
}

void Ec::recv_msg()
{
    Ec *ec = current->reply;

    if (EXPECT_TRUE (ec->cont == ret_user_sysexit)) {
        ec->utcb->save (current->utcb);
        if (EXPECT_FALSE (ec->utcb->ti))
            delegate<true>();
    }

    else if (ec->cont == ret_user_iret)
        current->utcb->load_exc (&ec->regs);
    else if (ec->cont == ret_user_vmresume)
        current->utcb->load_vmx (&ec->regs);
    else if (ec->cont == ret_user_vmrun)
        current->utcb->load_svm (&ec->regs);

    ret_user_sysexit();
}

void Ec::wait_msg()
{
    Ec *ec = current->reply;

    current->cont = static_cast<void (*)()>(0);

    if (EXPECT_TRUE (ec)) {
        if (ec->clr_partner())
            ec->make_current();
        else
            Ec::activate (Sc::current->ec());
    }

    Sc::schedule (true);
}

void Ec::sys_reply()
{
    Ec *ec = current->reply;

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

    wait_msg();
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

    Pd *pd = new Pd (dst, r->sel());
    if (!dst->Space_obj::insert_root (pd)) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sel());
        delete pd;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    Crd crd = r->crd();
    pd->delegate_crd (dst, Crd (Crd::OBJ, 0, Crd::whole, 0), crd);

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

    Ec *ec = new Ec (dst, r->sel(), r->cpu(), r->utcb(), r->esp(), r->evt(), r->flags() & 1);

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

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE SC:%#lx EC:%#lx P:%#lx Q:%#lx", current, r->sel(), r->ec(), r->qpd().prio(), r->qpd().quantum());

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

    Sc *sc = new Sc (dst, r->sel(), ec, ec->cpu, r->qpd().prio(), r->qpd().quantum());

    if (!ec->set_sc (sc)) {
        trace (TRACE_ERROR, "%s: Cannot bind SC", __func__);
        delete sc;
        sys_finish<Sys_regs::BAD_CAP>();
    }

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

    if (ec->pd != dst) {
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

    Pd::current->revoke_crd (r->crd(), r->flags());

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_lookup()
{
    Sys_lookup *s = static_cast<Sys_lookup *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_LOOKUP T:%#x B:%#lx", current, s->crd().type(), s->crd().base());

    Pd::current->lookup_crd (s->crd());

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_recall()
{
    Sys_recall *r = static_cast<Sys_recall *>(current->sys_regs());

    Kobject *obj = Space_obj::lookup (r->ec()).obj();
    if (EXPECT_FALSE (obj->type() != Kobject::EC)) {
        trace (TRACE_ERROR, "%s: Non-EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    Ec *ec = static_cast<Ec *>(obj);

    if (!(ec->regs.hazard() & HZD_RECALL)) {

        ec->regs.set_hazard (HZD_RECALL);

        if (Cpu::id != ec->cpu && Ec::remote (ec->cpu) == ec)
            Lapic::send_ipi (ec->cpu, Lapic::DST_PHYSICAL, Lapic::DLV_FIXED, VEC_IPI_RRQ);
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_semctl()
{
    Sys_semctl *r = static_cast<Sys_semctl *>(current->sys_regs());

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
            r->set_status (Sys_regs::SUCCESS);
            current->cont = ret_user_sysexit;
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
    &Ec::sys_recall,
    &Ec::sys_semctl,
    &Ec::sys_assign_pci,
    &Ec::sys_assign_gsi,
    &Ec::sys_finish<Sys_regs::BAD_SYS>,
    &Ec::sys_finish<Sys_regs::BAD_SYS>,
    &Ec::sys_finish<Sys_regs::BAD_SYS>,
};

template void Ec::send_msg<Ec::ret_user_vmresume>();
template void Ec::send_msg<Ec::ret_user_vmrun>();
template void Ec::send_msg<Ec::ret_user_iret>();
