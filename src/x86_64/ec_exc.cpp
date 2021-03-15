/*
 * Execution Context
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

#include "counter.hpp"
#include "ec_arch.hpp"
#include "fpu.hpp"
#include "gdt.hpp"
#include "mca.hpp"

void Ec::fpu_load()
{
    assert (fpu);

    if (is_vcpu())
        regs.fpu_ctrl (true);

    fpu->load();

    regs.hazard.set (Hazard::FPU);
}

void Ec::fpu_save()
{
    assert (fpu);

    if (is_vcpu())
        regs.fpu_ctrl (false);

    fpu->save();

    regs.hazard.clr (Hazard::FPU);
}

bool Ec_arch::handle_exc_gp (Exc_regs *)
{
    if (Cpu::hazard & Hazard::TR) {
        Cpu::hazard &= ~Hazard::TR;
        Tss::load();
        return true;
    }

    return false;
}

bool Ec_arch::handle_exc_pf (Exc_regs *r)
{
    auto const pfa { r->cr2 };
    auto const hst { regs.get_hst() };

    if (r->err & BIT (2))       // User-mode access
        return pfa < Space_hst::selectors() << PAGE_BITS && hst->loc[Cpu::id].share_from (hst->hptp, pfa, Space_hst::selectors() << PAGE_BITS);

    if (pfa >= LINK_ADDR && pfa < MMAP_CPU && hst->loc[Cpu::id].share_from_master (pfa))
        return true;

    // Kernel fault in PIO space
    if (pfa >= MMAP_SPC_PIO && pfa <= MMAP_SPC_PIO_E && hst->loc[Cpu::id].share_from (hst->hptp, pfa, MMAP_CPU))
        return true;

    // Convert #PF in I/O bitmap to #GP(0)
    if (r->user() && pfa >= MMAP_SPC_PIO && pfa <= MMAP_SPC_PIO_E) {
        r->vec = EXC_GP;
        r->err = r->cr2 = 0;
        send_msg<ret_user_exception> (this);
    }

    return false;
}

void Ec_arch::handle_exc (Exc_regs *r)
{
    Ec *const self = current;

    switch (r->vec) {

        case EXC_NM:
            if (switch_fpu (self))
                return;
            break;

        case EXC_GP:
            if (static_cast<Ec_arch *>(self)->handle_exc_gp (r))
                return;
            break;

        case EXC_PF:
            if (static_cast<Ec_arch *>(self)->handle_exc_pf (r))
                return;
            break;

        case EXC_MC:
            Mca::handler();
            break;
    }

    if (r->user())
        send_msg<ret_user_exception> (self);

    self->kill ("Unhandled exception");
}
