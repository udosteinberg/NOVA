/*
 * Execution Context (SVM)
 *
 * Copyright (C) 2009, Udo Steinberg <udo@hypervisor.org>
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
#include "svm.h"

void Ec::svm_handler()
{
    asm volatile ("vmload" : : "a" (Buddy::ptr_to_phys (Vmcb::root)));

    mword reason = static_cast<mword>(current->regs.vmcb->exitcode);

    switch (reason) {
        case -1ul:  reason = NUM_VMM - 1; break;    // Invalid state
        case 0x400: reason = NUM_VMM - 2; break;    // NPT
    }

    Counter::pre[reason + NUM_EXC]++;

    switch (reason) {
        case 0x60:      // EXTINT
            asm volatile ("sti; nop; cli" : : : "memory");
            ret_user_vmrun();
    }

    current->regs.dst_portal = reason + NUM_EXC;

    send_svm_msg();
}
