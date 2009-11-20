/*
 * System-Call Interface
 *
 * Copyright (C) 2007-2009, Udo Steinberg <udo@hypervisor.org>
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

#include "counter.h"
#include "ec.h"
#include "gsi.h"
#include "hip.h"
#include "mtd.h"
#include "pd.h"
#include "pt.h"
#include "sc.h"
#include "sm.h"
#include "stdio.h"
#include "utcb.h"
#include "vmx.h"

void Ec::sys_finish (Sys_regs *param, Sys_regs::Status status)
{
    param->set_status (status);

    ret_user_sysexit();
}

void Ec::recv_ipc_msg (mword ip, unsigned flags)
{
    Sys_ipc_recv *r = static_cast<Sys_ipc_recv *>(&regs);

    r->set_ip (ip);

    if (EXPECT_FALSE (flags & Sys_ipc_send::DISABLE_DONATION)) {
        sc->ready_enqueue();
        ret_user_sysexit();
    }

    reply = Capability (current);

    make_current();
}

void Ec::send_vmx_msg()
{
    Exc_regs *r = &current->regs;

    Capability cap = Space_obj::lookup (current->evt + r->dst_portal);

    Kobject *obj = cap.obj();
    if (EXPECT_FALSE (obj->type() != Kobject::PT))
        current->kill (r, "VMX PT");

    Pt *pt = static_cast<Pt *>(obj);
    Ec *ec = pt->ec;

    if (!Atomic::test_clr_bit<false>(ec->wait, 0))
        ec->block (send_vmx_msg);

    current->continuation = ret_user_vmresume;

    ec->utcb->load_vmx (r, pt->mtd);
    ec->utcb->pid = pt->node.base;
    ec->recv_ipc_msg (pt->ip);
}

void Ec::send_svm_msg()
{
    Exc_regs *r = &current->regs;

    Capability cap = Space_obj::lookup (current->evt + r->dst_portal);

    Kobject *obj = cap.obj();
    if (EXPECT_FALSE (obj->type() != Kobject::PT))
        current->kill (r, "SVM PT");

    Pt *pt = static_cast<Pt *>(obj);
    Ec *ec = pt->ec;

    if (!Atomic::test_clr_bit<false>(ec->wait, 0))
        ec->block (send_svm_msg);

    current->continuation = ret_user_vmrun;

    ec->utcb->load_svm (r, pt->mtd);
    ec->utcb->pid = pt->node.base;
    ec->recv_ipc_msg (pt->ip);
}

void Ec::send_exc_msg()
{
    Exc_regs *r = &current->regs;

    Capability cap = Space_obj::lookup (current->evt + r->dst_portal);

    Kobject *obj = cap.obj();
    if (EXPECT_FALSE (obj->type() != Kobject::PT))
        current->kill (r, "EXC PT");

    Pt *pt = static_cast<Pt *>(obj);
    Ec *ec = pt->ec;

    if (!Atomic::test_clr_bit<false>(ec->wait, 0))
        ec->block (send_exc_msg);

    current->continuation = ret_user_iret;

    ec->utcb->load_exc (r, pt->mtd);
    ec->utcb->pid = pt->node.base;
    ec->recv_ipc_msg (pt->ip);
}

void Ec::sys_ipc_call()
{
    Sys_ipc_send *s = static_cast<Sys_ipc_send *>(&current->regs);

    Capability cap = Space_obj::lookup (s->pt());

    Kobject *obj = cap.obj();
    if (EXPECT_FALSE (obj->type() != Kobject::PT))
        sys_finish (s, Sys_regs::BAD_CAP);

    Pt *pt = static_cast<Pt *>(obj);
    Ec *ec = pt->ec;

    if (EXPECT_FALSE (current->cpu != ec->cpu))
        sys_finish (s, Sys_regs::BAD_CPU);

    if (EXPECT_FALSE (!Atomic::test_clr_bit<false>(ec->wait, 0))) {
        if (EXPECT_FALSE (s->flags() & Sys_ipc_send::DISABLE_BLOCKING))
            sys_finish (s, Sys_regs::TIMEOUT);
        ec->block (sys_ipc_call);
    }

    mword *item = current->utcb->save (ec->utcb, s->mtd().untyped());

    unsigned long typed = s->mtd().typed();
    if (EXPECT_FALSE (typed))
        ec->pd->delegate_items (ec->utcb->crd, item, typed);

    current->continuation = ret_user_sysexit;

    ec->utcb->pid = pt->node.base;
    ec->recv_ipc_msg (pt->ip, s->flags());
}

void Ec::sys_ipc_reply()
{
    Sys_ipc_repl *s = static_cast<Sys_ipc_repl *>(&current->regs);

    Ec *ec = static_cast<Ec *>(current->reply.obj());

    if (EXPECT_TRUE (ec)) {

        Utcb *utcb = current->utcb;

        mword *item;
        if (EXPECT_TRUE (ec->continuation == ret_user_sysexit))
            item = utcb->save (ec->utcb, s->mtd().untyped());
        else if (ec->continuation == ret_user_iret)
            item = utcb->save_exc (&ec->regs, s->mtd());
        else if (ec->continuation == ret_user_vmresume)
            item = utcb->save_vmx (&ec->regs, s->mtd());
        else if (ec->continuation == ret_user_vmrun)
            item = utcb->save_svm (&ec->regs, s->mtd());
        else
            UNREACHED;

        unsigned long typed = s->mtd().typed();
        if (EXPECT_FALSE (typed)) {
            Crd crd = ec->continuation == ret_user_sysexit ? ec->utcb->crd : Crd (Crd::MEM, 0, Crd::whole);
            ec->pd->delegate_items (crd, item, typed);
        }

        current->reply = Capability();
    }

    current->continuation = ret_user_sysexit;

    /*
     * Ready to handle next request. If there is another thread already waiting
     * to rendezvous with us, then release that thread. If that thread has
     * a higher priority than us, ready_enqueue will set a schedule hazard
     * and we will reschedule on our way out of the kernel. We cannot schedule
     * here; we must return a potentially donated SC first (which happens by
     * calling make_current below).
     */
    current->wait = 1;
    current->release();

    if (EXPECT_TRUE (ec))
        ec->make_current();

    Sc::schedule (true);
}

void Ec::sys_create_pd()
{
    Sys_create_pd *r = static_cast<Sys_create_pd *>(&current->regs);

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE PD:%#lx UTCB:%#lx VM:%u", current, r->pd(), r->utcb(), r->flags() & 1);

    if (EXPECT_FALSE (r->utcb() >= LINK_ADDR || r->utcb() & PAGE_MASK)) {
        trace (TRACE_ERROR, "%s: Invalid UTCB address", __func__);
        sys_finish (r, Sys_regs::BAD_MEM);
    }

    // XXX: Check that CPU is online
    if (EXPECT_FALSE (r->cpu() >= NUM_CPU)) {
        trace (TRACE_ERROR, "%s: Invalid CPU", __func__);
        sys_finish (r, Sys_regs::BAD_CPU);
    }

    bool vm = r->flags() & 1;

    if (vm && !(Hip::feature() & (Hip::FEAT_VMX | Hip::FEAT_SVM))) {
        trace (TRACE_ERROR, "%s: Unsupported feature", __func__);
        sys_finish (r, Sys_regs::BAD_FTR);
    }

    Pd *pd = new Pd (vm);
    if (!Pd::current->Space_obj::insert (r->pd(), Capability (pd))) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->pd());
        delete pd;
        sys_finish (r, Sys_regs::BAD_CAP);
    }

    Ec *ec = new Ec (pd, r->cpu(), r->utcb());
    Sc *sc = new Sc (ec, r->cpu(), r->qpd().prio(), r->qpd().quantum());
    sc->ready_enqueue();

    pd->Space_obj::insert (NUM_EXC + 0, Capability (ec));
    pd->Space_obj::insert (NUM_EXC + 1, Capability (sc));

    pd->delegate (Crd (Crd::OBJ, 0, Crd::whole), r->crd());

    sys_finish (r, Sys_regs::SUCCESS);
}

void Ec::sys_create_ec()
{
    Sys_create_ec *r = static_cast<Sys_create_ec *>(&current->regs);

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE EC:%#lx CPU:%#lx UTCB:%#lx ESP:%#lx EVT:%#lx", current, r->ec(), r->cpu(), r->utcb(), r->esp(), r->evt());

    if (EXPECT_FALSE (r->utcb() >= LINK_ADDR || r->utcb() & PAGE_MASK)) {
        trace (TRACE_ERROR, "%s: Invalid UTCB address", __func__);
        sys_finish (r, Sys_regs::BAD_MEM);
    }

    // XXX: Check that CPU is online
    if (EXPECT_FALSE (r->cpu() >= NUM_CPU)) {
        trace (TRACE_ERROR, "%s: Invalid CPU", __func__);
        sys_finish (r, Sys_regs::BAD_CPU);
    }

    Ec *ec = new Ec (Pd::current, r->cpu(), r->utcb(), r->evt(), r->utcb() ? 1 : 0);
    if (!Pd::current->Space_obj::insert (r->ec(), Capability (ec))) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->ec());
        delete ec;
        sys_finish (r, Sys_regs::BAD_CAP);
    }

    static_cast<Sys_ipc_recv *>(&ec->regs)->set_sp (r->esp());

    sys_finish (r, Sys_regs::SUCCESS);
}

void Ec::sys_create_sc()
{
    Sys_create_sc *r = static_cast<Sys_create_sc *>(&current->regs);

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE SC:%#lx EC:%#lx P:%#lx Q:%#lx", current, r->sc(), r->ec(), r->qpd().prio(), r->qpd().quantum());

    Kobject *obj = Space_obj::lookup (r->ec()).obj();
    if (EXPECT_FALSE (obj->type() != Kobject::EC)) {
        trace (TRACE_ERROR, "%s: Non-EC CAP (%#lx)", __func__, r->ec());
        sys_finish (r, Sys_regs::BAD_CAP);
    }

    Ec *ec = static_cast<Ec *>(obj);

    if (ec->sc)
        sys_finish (r, Sys_regs::BAD_CAP);

    Sc *sc = new Sc (ec, ec->cpu, r->qpd().prio(), r->qpd().quantum());
    if (!Pd::current->Space_obj::insert (r->sc(), Capability (sc))) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sc());
        delete sc;
        sys_finish (r, Sys_regs::BAD_CAP);
    }

    if (!ec->utcb)
        sc->ready_enqueue();

    sys_finish (r, Sys_regs::SUCCESS);
}

void Ec::sys_create_pt()
{
    Sys_create_pt *r = static_cast<Sys_create_pt *>(&current->regs);

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE PT:%#lx EC:%#lx EIP:%#lx", current, r->pt(), r->ec(), r->eip());

    if (EXPECT_FALSE (r->eip() >= LINK_ADDR)) {
        trace (TRACE_ERROR, "%s: Invalid EIP", __func__);
        sys_finish (r, Sys_regs::BAD_MEM);
    }

    Kobject *obj = Space_obj::lookup (r->ec()).obj();
    if (EXPECT_FALSE (obj->type() != Kobject::EC)) {
        trace (TRACE_ERROR, "%s: Non-EC CAP (%#lx)", __func__, r->ec());
        sys_finish (r, Sys_regs::BAD_CAP);
    }

    Ec *ec = static_cast<Ec *>(obj);

    Pt *pt = new Pt (Pd::current, r->pt(), ec, r->mtd(), r->eip());
    if (!Pd::current->Space_obj::insert (Capability (pt), 0, &pt->node)) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->pt());
        delete pt;
        sys_finish (r, Sys_regs::BAD_CAP);
    }

    sys_finish (r, Sys_regs::SUCCESS);
}

void Ec::sys_create_sm()
{
    Sys_create_sm *r = static_cast<Sys_create_sm *>(&current->regs);

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE SM:%#lx CNT:%lu", current, r->sm(), r->cnt());

    Sm *sm = new Sm (Pd::current, r->sm(), r->cnt());
    if (!Pd::current->Space_obj::insert (Capability (sm), 0, &sm->node)) {
        trace (TRACE_ERROR, "%s: Non-NULL CAP (%#lx)", __func__, r->sm());
        delete sm;
        sys_finish (r, Sys_regs::BAD_CAP);
    }

    sys_finish (r, Sys_regs::SUCCESS);
}

void Ec::sys_revoke()
{
    Sys_revoke *r = static_cast<Sys_revoke *>(&current->regs);

    trace (TRACE_SYSCALL, "EC:%p SYS_REVOKE", current);

    Pd::revoke (r->crd(), r->flags());

    sys_finish (r, Sys_regs::SUCCESS);
}

void Ec::sys_recall()
{
    Sys_recall *r = static_cast<Sys_recall *>(&current->regs);

    Kobject *obj = Space_obj::lookup (r->ec()).obj();
    if (EXPECT_FALSE (obj->type() != Kobject::EC)) {
        trace (TRACE_ERROR, "%s: Non-EC CAP (%#lx)", __func__, r->ec());
        sys_finish (r, Sys_regs::BAD_CAP);
    }

    static_cast<Ec *>(obj)->hazard |= Cpu::HZD_RECALL;

    sys_finish (r, Sys_regs::SUCCESS);
}

void Ec::sys_semctl()
{
    Sys_semctl *r = static_cast<Sys_semctl *>(&current->regs);

    Kobject *obj = Space_obj::lookup (r->sm()).obj();
    if (EXPECT_FALSE (obj->type() != Kobject::SM)) {
        trace (TRACE_ERROR, "%s: Non-SM CAP (%#lx)", __func__, r->sm());
        sys_finish (r, Sys_regs::BAD_CAP);
    }

    Sm *sm = static_cast<Sm *>(obj);

    switch (r->flags() & 1) {
        case Sys_semctl::DN:
            if (sm->node.pd == &Pd::kern)
                Gsi::unmask (sm->node.base);
            r->set_status (Sys_regs::SUCCESS);
            current->continuation = ret_user_sysexit;
            sm->dn();
            break;
        case Sys_semctl::UP:
            sm->up();
            break;
    }

    sys_finish (r, Sys_regs::SUCCESS);
}

void Ec::exp_debug()
{
    Exp_debug *s = static_cast<Exp_debug *>(&current->regs);

    switch (s->func()) {
        case 0:
            Counter::dump();
            break;
        default:
            sys_finish (s, Sys_regs::BAD_CAP);
    }

    sys_finish (s, Sys_regs::SUCCESS);
}

void Ec::exp_lookup()
{
    Exp_lookup *s = static_cast<Exp_lookup *>(&current->regs);

    trace (TRACE_SYSCALL, "EC:%p EXP_LOOKUP PT:%#lx", current, s->pt());

    Kobject *obj = Space_obj::lookup (s->pt()).obj();
    if (EXPECT_FALSE (obj->type() != Kobject::PT)) {
        trace (TRACE_ERROR, "%s: Non-PT CAP (%#lx)", __func__, s->pt());
        sys_finish (s, Sys_regs::BAD_CAP);
    }

    Pt *pt = static_cast<Pt *>(obj);

    if (EXPECT_FALSE (Pd::current != pt->node.pd))
        sys_finish (s, Sys_regs::BAD_CAP);

    s->set_id (pt->node.base);

    sys_finish (s, Sys_regs::SUCCESS);
}

void Ec::syscall_handler (uint8 number)
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

    // Experimental functionality
    if (EXPECT_TRUE (number == Sys_regs::DEBUG))
        exp_debug();
    if (EXPECT_TRUE (number == Sys_regs::LOOKUP))
        exp_lookup();

    sys_finish (&current->regs, Sys_regs::BAD_SYS);
}
