/*
 * System-Call Interface
 *
 * Copyright (C) 2007-2010, Udo Steinberg <udo@hypervisor.org>
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

    // Helping
    ec->make_current();
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

    if (!Atomic::test_clr_bit<false>(ec->wait, 0))
        ec->help (send_msg<C>);

    current->continuation = C;
    current->set_partner (ec);

    ec->mtr = pt->mtd;
    ec->utcb->pid = pt->vma.base;
    ec->regs.set_ip (pt->ip);

    assert (ec->continuation == recv_msg);
    ec->make_current();
}

void Ec::sys_ipc_call()
{
    Sys_ipc_send *s = static_cast<Sys_ipc_send *>(&current->regs);

    Kobject *obj = Space_obj::lookup (s->pt()).obj();
    if (EXPECT_FALSE (obj->type() != Kobject::PT))
        sys_finish<Sys_regs::BAD_CAP>();

    Pt *pt = static_cast<Pt *>(obj);
    Ec *ec = pt->ec;

    if (EXPECT_FALSE (current->cpu != ec->cpu))
        sys_finish<Sys_regs::BAD_CPU>();

    if (EXPECT_FALSE (!Atomic::test_clr_bit<false>(ec->wait, 0))) {
        if (EXPECT_FALSE (s->flags() & Sys_ipc_send::DISABLE_BLOCKING))
            sys_finish<Sys_regs::TIMEOUT>();
        ec->help (sys_ipc_call);
    }

    current->continuation = ret_user_sysexit;
    current->set_partner (ec);

    ec->mtr = s->mtd();
    ec->utcb->pid = pt->vma.base;       // Set dst portal
    ec->regs.set_ip (pt->ip);           // Set entry EIP

    assert (ec->continuation == recv_msg);
    ec->make_current();
}

void Ec::recv_msg()
{
    Ec *ec = static_cast<Ec *>(current->reply.obj());
    assert (ec);

    Mtd mtd = current->mtr;
    Exc_regs *r = &ec->regs;

    Utcb *src = ec->utcb;
    Utcb *dst = current->utcb;

    if (EXPECT_TRUE (ec->continuation == ret_user_sysexit)) {
        src->save (dst, mtd);

        unsigned long ui = mtd.ui();
        unsigned long ti = mtd.ti();

        if (EXPECT_FALSE (ti))
            current->pd->delegate_items (ec->pd, dst->crd, src->ptr (ui), dst->ptr (ui), ti);
    }

    else if (ec->continuation == ret_user_iret)
        dst->load_exc (r, mtd);
    else if (ec->continuation == ret_user_vmresume)
        dst->load_vmx (r, mtd);
    else if (ec->continuation == ret_user_vmrun)
        dst->load_svm (r, mtd);

    ret_user_sysexit();
}

void Ec::sys_ipc_reply()
{
    Sys_ipc_repl *s = static_cast<Sys_ipc_repl *>(&current->regs);

    Ec *ec = static_cast<Ec *>(current->reply.obj());

    if (EXPECT_TRUE (ec)) {

        Utcb *src = current->utcb, *dst = ec->utcb;

        unsigned long ui = s->mtd().ui();

        if (EXPECT_TRUE (ec->continuation == ret_user_sysexit))
            src->save (dst, s->mtd());
        else if (ec->continuation == ret_user_iret)
            ui = src->save_exc (&ec->regs, s->mtd());
        else if (ec->continuation == ret_user_vmresume)
            ui = src->save_vmx (&ec->regs, s->mtd());
        else if (ec->continuation == ret_user_vmrun)
            ui = src->save_svm (&ec->regs, s->mtd());

        unsigned long ti = s->mtd().ti();
        if (EXPECT_FALSE (ti)) {
            if (ec->continuation == ret_user_sysexit)
                ec->pd->delegate_items (current->pd, dst->crd, src->ptr (ui), dst->ptr (ui), ti);
            else
                ec->pd->delegate_items (current->pd, Crd (Crd::MEM, 0, Crd::whole, 0), src->ptr (ui), 0, ti);
        }

        current->continuation = recv_msg;
        current->wait = 1;

        if (ec->clr_partner())
            ec->make_current();

    } else {

        current->continuation = recv_msg;
        current->wait = 1;
    }

    Sc::schedule (!ec);
}

void Ec::sys_create_pd()
{
    Sys_create_pd *r = static_cast<Sys_create_pd *>(&current->regs);

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE PD:%#lx UTCB:%#lx VM:%u", current, r->pd(), r->utcb(), r->flags() & 1);

    if (EXPECT_FALSE (!Hip::cpu_online (r->cpu()))) {
        trace (TRACE_ERROR, "%s: Invalid CPU (%#lx)", __func__, r->cpu());
        sys_finish<Sys_regs::BAD_CPU>();
    }

    if (EXPECT_FALSE (r->utcb() >= LINK_ADDR || r->utcb() & PAGE_MASK)) {
        trace (TRACE_ERROR, "%s: Invalid UTCB address (%#lx)", __func__, r->utcb());
        sys_finish<Sys_regs::BAD_MEM>();
    }

    if ((r->flags() & 0x1) && !(Hip::feature() & (Hip::FEAT_VMX | Hip::FEAT_SVM))) {
        trace (TRACE_ERROR, "%s: VM not supported", __func__);
        sys_finish<Sys_regs::BAD_FTR>();
    }

    Pd *pd = new Pd (r->flags());
    if (!Pd::current->Space_obj::insert (r->pd(), Capability (pd))) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->pd());
        delete pd;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    Ec *ec = new Ec (pd, r->cpu(), r->utcb(), 0, 0, false);
    Sc *sc = new Sc (ec, r->cpu(), r->qpd().prio(), r->qpd().quantum());

    ec->set_sc (sc);
    pd->Space_obj::insert (NUM_EXC + 0, Capability (ec));
    pd->Space_obj::insert (NUM_EXC + 1, Capability (sc));

    Crd crd = r->crd();
    pd->delegate (current->pd, Crd (Crd::OBJ, 0, Crd::whole, 0), crd);
    pd->insert_utcb (r->utcb());

    // Enqueue SC only after all the caps have been mapped
    sc->remote_enqueue();

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_create_ec()
{
    Sys_create_ec *r = static_cast<Sys_create_ec *>(&current->regs);

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE EC:%#lx CPU:%#lx UTCB:%#lx ESP:%#lx EVT:%#lx", current, r->ec(), r->cpu(), r->utcb(), r->esp(), r->evt());

    if (EXPECT_FALSE (!Hip::cpu_online (r->cpu()))) {
        trace (TRACE_ERROR, "%s: Invalid CPU (%#lx)", __func__, r->cpu());
        sys_finish<Sys_regs::BAD_CPU>();
    }

    if (EXPECT_FALSE (r->utcb() >= LINK_ADDR || r->utcb() & PAGE_MASK || !Pd::current->insert_utcb (r->utcb()))) {
        trace (TRACE_ERROR, "%s: Invalid UTCB address (%#lx)", __func__, r->utcb());
        sys_finish<Sys_regs::BAD_MEM>();
    }

    Ec *ec = new Ec (Pd::current, r->cpu(), r->utcb(), r->esp(), r->evt(), r->flags() & 1);
    if (!Pd::current->Space_obj::insert (r->ec(), Capability (ec))) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->ec());
        delete ec;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_create_sc()
{
    Sys_create_sc *r = static_cast<Sys_create_sc *>(&current->regs);

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE SC:%#lx EC:%#lx P:%#lx Q:%#lx", current, r->sc(), r->ec(), r->qpd().prio(), r->qpd().quantum());

    Kobject *obj = Space_obj::lookup (r->ec()).obj();
    if (EXPECT_FALSE (obj->type() != Kobject::EC)) {
        trace (TRACE_ERROR, "%s: Non-EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    Ec *ec = static_cast<Ec *>(obj);
    Sc *sc = new Sc (ec, ec->cpu, r->qpd().prio(), r->qpd().quantum());

    if (!ec->set_sc (sc)) {
        trace (TRACE_ERROR, "%s: Cannot attach SC", __func__);
        delete sc;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    if (!Pd::current->Space_obj::insert (r->sc(), Capability (sc))) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sc());
        delete sc;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    sc->remote_enqueue();

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_create_pt()
{
    Sys_create_pt *r = static_cast<Sys_create_pt *>(&current->regs);

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

    Pt *pt = new Pt (ec, r->mtd(), r->eip(), Pd::current, r->pt());

    if (!Pd::current->Space_obj::insert (&pt->vma, Capability (pt))) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->pt());
        delete pt;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_create_sm()
{
    Sys_create_sm *r = static_cast<Sys_create_sm *>(&current->regs);

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE SM:%#lx CNT:%lu", current, r->sm(), r->cnt());

    Sm *sm = new Sm (r->cnt(), Pd::current, r->sm());

    if (!Pd::current->Space_obj::insert (&sm->vma, Capability (sm))) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sm());
        delete sm;
        sys_finish<Sys_regs::BAD_CAP>();
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_revoke()
{
    Sys_revoke *r = static_cast<Sys_revoke *>(&current->regs);

    trace (TRACE_SYSCALL, "EC:%p SYS_REVOKE", current);

    Pd::revoke (r->crd(), r->flags());

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_recall()
{
    Sys_recall *r = static_cast<Sys_recall *>(&current->regs);

    Kobject *obj = Space_obj::lookup (r->ec()).obj();
    if (EXPECT_FALSE (obj->type() != Kobject::EC)) {
        trace (TRACE_ERROR, "%s: Non-EC CAP (%#lx)", __func__, r->ec());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    Ec *ec = static_cast<Ec *>(obj);

    if (!(ec->hazard & Cpu::HZD_RECALL)) {

        Atomic::set_mask<true>(ec->hazard, Cpu::HZD_RECALL);

        // XXX: We only need to send the IPI if we were the first to set the
        // recall hazard and if the target EC is currently running on its CPU.
        if (Cpu::id != ec->cpu)
            Lapic::send_ipi (ec->cpu, Lapic::DST_PHYSICAL, Lapic::DLV_FIXED, VEC_IPI_RRQ);
    }

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_semctl()
{
    Sys_semctl *r = static_cast<Sys_semctl *>(&current->regs);

    Kobject *obj = Space_obj::lookup (r->sm()).obj();
    if (EXPECT_FALSE (obj->type() != Kobject::SM)) {
        trace (TRACE_ERROR, "%s: Non-SM CAP (%#lx)", __func__, r->sm());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    Sm *sm = static_cast<Sm *>(obj);

    switch (r->flags() & 1) {
        case Sys_semctl::DN:
            if (sm->vma.pd == &Pd::kern)
                Gsi::unmask (sm->vma.base);
            r->set_status (Sys_regs::SUCCESS);
            current->continuation = ret_user_sysexit;
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
    Sys_assign_pci *r = static_cast<Sys_assign_pci *>(&current->regs);

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

    Pd *pd = static_cast<Pd *>(obj);

    if (!pd->dpt) {
        trace (TRACE_ERROR, "%s: DMA not supported", __func__);
        sys_finish<Sys_regs::BAD_CAP>();
    }

    dmar->assign (r->vf() ? r->vf() : r->pf(), pd);

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::sys_assign_gsi()
{
    Sys_assign_gsi *r = static_cast<Sys_assign_gsi *>(&current->regs);

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

    if (EXPECT_FALSE (sm->vma.pd != &Pd::kern)) {
        trace (TRACE_ERROR, "%s: Non-VEC SM (%#lx)", __func__, r->sm());
        sys_finish<Sys_regs::BAD_CAP>();
    }

    r->set_msi (Gsi::set (sm->vma.base, r->cpu(), r->rid()));

    sys_finish<Sys_regs::SUCCESS>();
}

void Ec::handle_sys (uint8 number)
{
    // This is actually faster than a switch
    if (EXPECT_TRUE (number == Sys_regs::MSG_CALL))
        sys_ipc_call();
    if (EXPECT_TRUE (number == Sys_regs::MSG_REPLY))
        sys_ipc_reply();
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
#if 0
    if (EXPECT_TRUE (number == Sys_regs::REVOKE))
        sys_revoke();
#endif
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
