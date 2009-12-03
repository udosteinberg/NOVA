/*
 * Secure Virtual Machine (SVM)
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

#include "cpu.h"
#include "hip.h"
#include "msr.h"
#include "stdio.h"
#include "svm.h"

Vmcb *              Vmcb::root;
unsigned            Vmcb::asid_ctr;
Vmcb::svm_feature   Vmcb::feature;

Vmcb::Vmcb (mword nptp) : asid (++asid_ctr), npt_control (1), npt_cr3 (nptp), efer (Cpu::EFER_SVME), g_pat (0x7040600070406ull)
{
    char *bitmap = reinterpret_cast<char *>(this) + PAGE_SIZE;

    memset (bitmap, ~0u, 3 * PAGE_SIZE);
    base_io = base_msr = Buddy::ptr_to_phys (bitmap);
    int_control = (1ul << 24);
}

void Vmcb::init()
{
    if (!Cpu::feature (Cpu::FEAT_SVM) || !Vmcb::feature.np) {
        Hip::remove (Hip::FEAT_SVM);
        return;
    }

    Msr::write (Msr::IA32_EFER, Msr::read<uint32>(Msr::IA32_EFER) | Cpu::EFER_SVME);
    Msr::write (Msr::AMD_SVM_HSAVE_PA, Buddy::ptr_to_phys (new Vmcb));
}
