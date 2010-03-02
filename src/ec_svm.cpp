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

#include "ec.h"
#include "svm.h"

void Ec::handle_svm()
{
    asm volatile ("vmload" : : "a" (Buddy::ptr_to_phys (Vmcb::root)));

    mword reason = static_cast<mword>(current->regs.vmcb->exitcode);

    switch (reason) {
        case -1ul:  reason = NUM_VMI - 3; break;    // Invalid state
        case 0x400: reason = NUM_VMI - 4; break;    // NPT
    }

    Counter::vmi[reason]++;

    switch (reason) {
        case 0x60:      // EXTINT
            asm volatile ("sti; nop; cli" : : : "memory");
            ret_user_vmrun();
    }

    current->regs.dst_portal = reason;

    send_msg<ret_user_vmrun>();
}
