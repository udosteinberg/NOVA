/*
 * Execution Context (SVM)
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

#include "ec.h"
#include "svm.h"
#include "vtlb.h"

uint8 Ec::ifetch (mword virt)
{
    mword phys, attr = 0, type = 0;
    uint8 opcode;

    if (!Vtlb::walk (&current->regs, virt, phys, attr, type))
        panic ("%s tlb failure eip=%#lx", __func__, virt);

    if (User::peek (reinterpret_cast<uint8 *>(phys), opcode) != ~0UL)
        panic ("%s ifetch failure eip=%#lx", __func__, virt);

    return opcode;
}

void Ec::svm_exception (mword reason)
{
    if (current->regs.vmcb->exitintinfo & 0x80000000) {

        mword t = static_cast<mword>(current->regs.vmcb->exitintinfo) >> 8 & 0x7;
        mword v = static_cast<mword>(current->regs.vmcb->exitintinfo) & 0xff;

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

        case 0x4e:          // #PF
            mword err = static_cast<mword>(current->regs.vmcb->exitinfo1);
            mword cr2 = static_cast<mword>(current->regs.vmcb->exitinfo2);

            switch (Vtlb::miss (&current->regs, cr2, err)) {

                case Vtlb::GPA_HPA:
                    current->regs.nst_error = 0;
                    current->regs.dst_portal = NUM_VMI - 4;
                    break;

                case Vtlb::GLA_GPA:
                    current->regs.vmcb->cr2 = cr2;
                    current->regs.vmcb->inj_control = static_cast<uint64>(err) << 32 | 0x80000b0e;

                case Vtlb::SUCCESS:
                    ret_user_vmrun();
            }
    }

    send_msg<ret_user_vmrun>();
}

void Ec::svm_invlpg()
{
    mword virt = static_cast<mword>(current->regs.vmcb->cs.base + current->regs.vmcb->rip);

    current->regs.svm_update_shadows();

    assert (ifetch (virt) == 0xf && ifetch (virt + 1) == 0x1);

    uint8 mrm = ifetch (virt + 2);
    uint8 r_m = mrm & 7;

    unsigned len = 3;

    switch (mrm >> 6) {
        case 0: len += (r_m == 4 ? 1 : r_m == 5 ? 4 : 0); break;
        case 1: len += (r_m == 4 ? 2 : 1); break;
        case 2: len += (r_m == 4 ? 5 : 4); break;
    }

    current->regs.tlb_flush<Vmcb>(true);
    current->regs.vmcb->adjust_rip (len);
    ret_user_vmrun();
}

void Ec::svm_cr()
{
    mword virt = static_cast<mword>(current->regs.vmcb->cs.base + current->regs.vmcb->rip);

    current->regs.svm_update_shadows();

    assert (ifetch (virt) == 0xf);

    uint8 opc = ifetch (virt + 1);
    uint8 mrm = ifetch (virt + 2);

    unsigned len, gpr = mrm & 0x7, cr = mrm >> 3 & 0x7;

    switch (opc) {

        case 0x6:       // CLTS
            current->regs.write_cr<Vmcb> (0, current->regs.read_cr<Vmcb> (0) & ~Cpu::CR0_TS);
            len = 2;
            break;

        case 0x20:      // MOV from CR
            current->regs.svm_write_gpr (gpr, current->regs.read_cr<Vmcb>(cr));
            len = 3;
            break;

        case 0x22:      // MOV to CR
            current->regs.write_cr<Vmcb> (cr, current->regs.svm_read_gpr (gpr));
            len = 3;
            break;

        default:
            panic ("%s: decode failure eip=%#lx opc=%#x", __func__, virt, opc);
    }

    current->regs.vmcb->adjust_rip (len);
    ret_user_vmrun();
}

void Ec::handle_svm()
{
    asm volatile ("vmload" : : "a" (Buddy::ptr_to_phys (Vmcb::root)));

    current->regs.vmcb->tlb_control = 0;

    mword reason = static_cast<mword>(current->regs.vmcb->exitcode);

    switch (reason) {
        case -1ul:  reason = NUM_VMI - 3; break;    // Invalid state
        case 0x400: reason = NUM_VMI - 4; break;    // NPT
    }

    Counter::vmi[reason]++;

    switch (reason) {

        case 0x0 ... 0x1f:      // CR Access
            svm_cr();

        case 0x40 ... 0x5f:     // Exception
            svm_exception (reason);

        case 0x60:              // EXTINT
            asm volatile ("sti; nop; cli" : : : "memory");
            ret_user_vmrun();

        case 0x79:              // INVLPG
            svm_invlpg();
    }

    current->regs.dst_portal = reason;

    send_msg<ret_user_vmrun>();
}
