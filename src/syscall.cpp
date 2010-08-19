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

template <bool C, void (*F)()>
void Ec::delegate()
{
    Ec *ec = current->reply;
    assert (ec);

    Ec *src = C ? ec : current;
    Ec *dst = C ? current : ec;

    unsigned long ui = current->mtr.ui();
    unsigned long ti = current->mtr.ti();

    bool user = C || dst->cont == ret_user_sysexit;

    dst->pd->delegate_items (src->pd,
                             user ? dst->utcb->crd : Crd (Crd::MEM, 0, Crd::whole, 0),
                             src->utcb->ptr (ui),
                             user ? dst->utcb->ptr (ui) : 0,
                             ti);

    F();
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
        ec->mtr  = pt->mtd;
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
        ec->mtr  = s->mtd();
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
        ec->utcb->save (current->utcb, current->mtr);
        if (EXPECT_FALSE (current->mtr.ti()))
            delegate<true, ret_user_sysexit>();
    }

    else if (ec->cont == ret_user_iret)
        current->utcb->load_exc (&ec->regs, current->mtr);
    else if (ec->cont == ret_user_vmresume)
        current->utcb->load_vmx (&ec->regs, current->mtr);
    else if (ec->cont == ret_user_vmrun)
        current->utcb->load_svm (&ec->regs, current->mtr);

    ret_user_sysexit();
}

void Ec::wait_msg()
{
    Ec *ec = current->reply;

    current->cont = static_cast<void (*)()>(0);

    if (ec->clr_partner())
        ec->make_current();
    else
        Ec::activate (Sc::current->ec());

    Sc::schedule();
}

void Ec::sys_reply()
{
    Sys_reply *s = static_cast<Sys_reply *>(current->sys_regs());

    Ec *ec = current->reply;

    if (EXPECT_FALSE (!ec)) {
        current->cont = static_cast<void (*)()>(0);
        Sc::schedule (true);
    }

    Utcb *src = current->utcb;

    unsigned long ui = s->mtd().ui();

    if (EXPECT_TRUE (ec->cont == ret_user_sysexit))
        src->save (ec->utcb, s->mtd());
    else if (ec->cont == ret_user_iret)
        ui = src->save_exc (&ec->regs, s->mtd());
    else if (ec->cont == ret_user_vmresume)
        ui = src->save_vmx (&ec->regs, s->mtd());
    else if (ec->cont == ret_user_vmrun)
        ui = src->save_svm (&ec->regs, s->mtd());

    unsigned long ti = s->mtd().ti();
    if (EXPECT_TRUE (!ti))
        wait_msg();

    current->mtr = Mtd (ti, ui);
    delegate<false, wait_msg>();
}

void Ec::sys_create_pd()
{
    Sys_create_pd *r = static_cast<Sys_create_pd *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE PD:%#lx CPU:%#lx UTCB:%#lx", current, r->pd(), r->cpu(), r->utcb());

    if (EXPECT_FALSE (!Hip::cpu_online (r->cpu()))) {
        trace (TRACE_ERROR, "%s: Invalid CPU (%#lx)", __func__, r->cpu());
        sys_finish<Sys_regs::BAD_CPU>();
    }

    if (EXPECT_FALSE (!r->utcb() || r->utcb() >= LINK_ADDR || r->utcb() & PAGE_MASK)) {
        trace (TRACE_ERROR, "%s: Invalid UTCB address (%#lx)", __func__, r->utcb());
        sys_finish<Sys_regs::BAD_MEM>();
    }

    Pd *pd = new Pd (Pd::current, r->pd());
    if (!Space_obj::insert_root (pd)) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->pd());
        delete pd;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    Ec *ec = new Ec (pd, NUM_EXC + 0, pd, r->cpu(), r->utcb(), 0, 0, false);
    Sc *sc = new Sc (pd, NUM_EXC + 1, ec, r->cpu(), r->qpd().prio(), r->qpd().quantum());

    ec->set_sc (sc);
    Space_obj::insert_root (ec);
    Space_obj::insert_root (sc);

    Crd crd = r->crd();
    pd->delegate_item (current->pd, Crd (Crd::OBJ, 0, Crd::whole, 0), crd);
    pd->insert_utcb (r->utcb());

    // Enqueue SC only after all the caps have been mapped
    sc->remote_enqueue();

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_create_ec()
{
    Sys_create_ec *r = static_cast<Sys_create_ec *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE EC:%#lx CPU:%#lx UTCB:%#lx ESP:%#lx EVT:%#lx", current, r->ec(), r->cpu(), r->utcb(), r->esp(), r->evt());

    if (EXPECT_FALSE (!Hip::cpu_online (r->cpu()))) {
        trace (TRACE_ERROR, "%s: Invalid CPU (%#lx)", __func__, r->cpu());
        sys_finish<Sys_regs::BAD_CPU>();
    }

    if (EXPECT_FALSE (!r->utcb() && !(Hip::feature() & (Hip::FEAT_VMX | Hip::FEAT_SVM)))) {
        trace (TRACE_ERROR, "%s: VCPUs not supported", __func__);
        sys_finish<Sys_regs::BAD_FTR>();
    }

    if (EXPECT_FALSE (r->utcb() >= LINK_ADDR || r->utcb() & PAGE_MASK || !Pd::current->insert_utcb (r->utcb()))) {
        trace (TRACE_ERROR, "%s: Invalid UTCB address (%#lx)", __func__, r->utcb());
        sys_finish<Sys_regs::BAD_MEM>();
    }

    Ec *ec = new Ec (Pd::current, r->ec(), Pd::current, r->cpu(), r->utcb(), r->esp(), r->evt(), r->flags() & 1);
    if (!Space_obj::insert_root (ec)) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->ec());
        delete ec;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_create_sc()
{
    Sys_create_sc *r = static_cast<Sys_create_sc *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE SC:%#lx EC:%#lx P:%#lx Q:%#lx", current, r->sc(), r->ec(), r->qpd().prio(), r->qpd().quantum());

    Kobject *obj = Space_obj::lookup (r->ec()).obj();
    if (EXPECT_FALSE (obj->type() != Kobject::EC)) {
        trace (TRACE_ERROR, "%s: Non-EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    Ec *ec = static_cast<Ec *>(obj);
    Sc *sc = new Sc (Pd::current, r->sc(), ec, ec->cpu, r->qpd().prio(), r->qpd().quantum());

    if (!ec->set_sc (sc)) {
        trace (TRACE_ERROR, "%s: Cannot attach SC", __func__);
        delete sc;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    if (!Space_obj::insert_root (sc)) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sc());
        delete sc;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    sc->remote_enqueue();

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_create_pt()
{
    Sys_create_pt *r = static_cast<Sys_create_pt *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE PT:%#lx EC:%#lx EIP:%#lx", current, r->pt(), r->ec(), r->eip());

    if (EXPECT_FALSE (r->eip() >= LINK_ADDR)) {
        trace (TRACE_ERROR, "%s: Invalid EIP (%#lx)", __func__, r->eip());
        sys_finish<Sys_regs::BAD_MEM>();
    }

    Kobject *obj = Space_obj::lookup (r->ec()).obj();
    if (EXPECT_FALSE (obj->type() != Kobject::EC)) {
        trace (TRACE_ERROR, "%s: Non-EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    Ec *ec = static_cast<Ec *>(obj);

    Pt *pt = new Pt (Pd::current, r->pt(), ec, r->mtd(), r->eip());
    if (!Space_obj::insert_root (pt)) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->pt());
        delete pt;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_create_sm()
{
    Sys_create_sm *r = static_cast<Sys_create_sm *>(current->sys_regs());

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE SM:%#lx CNT:%lu", current, r->sm(), r->cnt());

    Sm *sm = new Sm (Pd::current, r->sm(), r->cnt());
    if (!Space_obj::insert_root (sm)) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sm());
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
    if (EXPECT_FALSE (cap.obj()->type() != Kobject::SM || (r->flags() & cap.acc()) != r->flags())) {
        trace (TRACE_ERROR, "%s: Non-SM CAP (%#lx)", __func__, r->sm());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    Sm *sm = static_cast<Sm *>(cap.obj());

    switch (r->flags() & 1) {
        case Sys_semctl::DN:
            if (sm->node_pd == &Pd::kern)
                Gsi::unmask (sm->node_base);
            r->set_status (Sys_regs::SUCCESS);
            current->cont = ret_user_sysexit;
            sm->dn();
            break;
        case Sys_semctl::UP:
            sm->up();
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
        trace (TRACE_ERROR, "%s: Invalid CPU (%#lx)", __func__, r->cpu());
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

    r->set_msi (Gsi::set (sm->node_base, r->cpu(), r->rid()));

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::handle_sys (uint8 number)
{
    // This is actually faster than a switch
    if (EXPECT_TRUE (number == Sys_regs::CALL))
        sys_call();
    if (EXPECT_TRUE (number == Sys_regs::REPLY))
        sys_reply();
    if (EXPECT_TRUE (number == Sys_regs::CREATE_PD))
        sys_create_pd();
    if (EXPECT_TRUE (number == Sys_regs::CREATE_EC))
        sys_create_ec();
    if (EXPECT_TRUE (number == Sys_regs::CREATE_SC))
        sys_create_sc();
    if (EXPECT_TRUE (number == Sys_regs::CREATE_PT))
        sys_create_pt();
    if (EXPECT_TRUE (number == Sys_regs::CREATE_SM))
        sys_create_sm();
    if (EXPECT_TRUE (number == Sys_regs::REVOKE))
        sys_revoke();
    if (EXPECT_TRUE (number == Sys_regs::LOOKUP))
        sys_lookup();
    if (EXPECT_TRUE (number == Sys_regs::RECALL))
        sys_recall();
    if (EXPECT_TRUE (number == Sys_regs::SEMCTL))
        sys_semctl();
    if (EXPECT_TRUE (number == Sys_regs::ASSIGN_PCI))
        sys_assign_pci();
    if (EXPECT_TRUE (number == Sys_regs::ASSIGN_GSI))
        sys_assign_gsi();

    sys_finish<Sys_regs::BAD_SYS>();
}

template void Ec::send_msg<Ec::ret_user_vmresume>();
template void Ec::send_msg<Ec::ret_user_vmrun>();
template void Ec::send_msg<Ec::ret_user_iret>();
