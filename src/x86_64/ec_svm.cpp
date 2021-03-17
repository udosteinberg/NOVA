/*
 * Execution Context (SVM)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "ec.hpp"
#include "svm.hpp"

void Ec::svm_exception (uint64_t reason)
{
    if (current->regs.vmcb->exitintinfo & 0x80000000) {

        auto const t { current->regs.vmcb->exitintinfo >> 8 & 0x7 };
        auto const v { current->regs.vmcb->exitintinfo & 0xff };

        if (t == 0 || (t == 3 && v != 3 && v != 4))
            current->regs.vmcb->inj_control = current->regs.vmcb->exitintinfo;
    }

    switch (reason) {

        default:
            current->regs.dst_portal = reason;
            break;

        case 0x47:          // #NM
            handle_exc_nm();
            ret_user_vmrun();
    }

    send_msg<ret_user_vmrun>();
}

void Ec::handle_svm()
{
    current->regs.vmcb->tlb_control = 0;

    auto reason { current->regs.vmcb->exitcode };

    switch (reason) {
        case -1UL:              // Invalid state
            reason = NUM_VMI - 3;
            break;
        case 0x400:             // NPT
            reason = NUM_VMI - 4;
            break;
    }

    switch (reason) {

        case 0x40 ... 0x5f:     // Exception
            svm_exception (reason);

        case 0x60:              // EXTINT
            asm volatile ("sti; nop; cli" : : : "memory");
            ret_user_vmrun();
    }

    current->regs.dst_portal = reason;

    send_msg<ret_user_vmrun>();
}
