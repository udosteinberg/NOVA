/*
 * Execution Context
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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
#include "extern.hpp"
#include "gdt.hpp"
#include "mca.hpp"
#include "stdio.hpp"

void Ec::load_fpu()
{
    if (!utcb)
        regs.fpu_ctrl (true);

    if (EXPECT_FALSE (!fpu))
        fpu = new (pd->fpu_cache) Fpu;

    fpu->load();
}

void Ec::save_fpu()
{
    if (!utcb)
        regs.fpu_ctrl (false);

    if (EXPECT_FALSE (!fpu))
        fpu = new (pd->fpu_cache) Fpu;

    fpu->save();
}

void Ec::transfer_fpu (Ec *ec)
{
    if (!(Cpu::hazard & Hazard::FPU)) {

        Fpu::enable();

        if (fpowner != this) {
            if (fpowner)
                fpowner->save_fpu();
            load_fpu();
        }
    }

    fpowner = ec;
}

void Ec::handle_exc_nm()
{
    Fpu::enable();

    if (current == fpowner)
        return;

    if (fpowner)
        fpowner->save_fpu();

    current->load_fpu();

    fpowner = current;
}

bool Ec::handle_exc_gp (Exc_regs *)
{
    if (Cpu::hazard & Hazard::TR) {
        Cpu::hazard &= ~Hazard::TR;
        Tss::load();
        return true;
    }

    return false;
}

bool Ec::handle_exc_pf (Exc_regs *r)
{
    auto const pfa { r->cr2 };
    auto const hst { current->regs.get_hst() };

    if (r->err & BIT (2))       // User-mode access
        return pfa < USER_ADDR && hst->loc[Cpu::id].share_from (hst->hptp, pfa, USER_ADDR);

    if (pfa >= LINK_ADDR && pfa < MMAP_CPU && hst->loc[Cpu::id].share_from_master (pfa))
        return true;

    // Kernel fault in PIO space
    if (pfa >= MMAP_SPC_PIO && pfa <= MMAP_SPC_PIO_E && hst->loc[Cpu::id].share_from (hst->hptp, pfa, MMAP_CPU))
        return true;

    // Convert #PF in I/O bitmap to #GP(0)
    if (r->user() && pfa >= MMAP_SPC_PIO && pfa <= MMAP_SPC_PIO_E) {
        r->vec = EXC_GP;
        r->err = r->cr2 = 0;
        send_msg<ret_user_iret>();
    }

    die ("#PF (kernel)", r);
}

void Ec::handle_exc (Exc_regs *r)
{
    switch (r->vec) {

        case EXC_NM:
            handle_exc_nm();
            return;

        case EXC_GP:
            if (handle_exc_gp (r))
                return;
            break;

        case EXC_PF:
            if (handle_exc_pf (r))
                return;
            break;

        case EXC_MC:
            Mca::handler();
            break;
    }

    if (r->user())
        send_msg<ret_user_iret>();

    die ("EXC", r);
}
