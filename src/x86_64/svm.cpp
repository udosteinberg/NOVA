/*
 * Secure Virtual Machine (SVM)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "arch.hpp"
#include "cmdline.hpp"
#include "cpu.hpp"
#include "hip.hpp"
#include "msr.hpp"
#include "stdio.hpp"
#include "svm.hpp"

Paddr       Vmcb::root;
unsigned    Vmcb::asid_ctr;
uint32      Vmcb::svm_version;
uint32      Vmcb::svm_feature;

Vmcb::Vmcb (mword bmp, mword nptp) : base_io (bmp), asid (++asid_ctr), int_control (1ul << 24), npt_cr3 (nptp), efer (EFER_SVME), g_pat (0x7040600070406ull)
{
    base_msr = Kmem::ptr_to_phys (Buddy::allocator.alloc (1, Buddy::FILL_1));
}

void Vmcb::init()
{
    if (!Cpu::feature (Cpu::FEAT_SVM)) {
        Hip::hip->clr_feature (Hip::FEAT_SVM);
        return;
    }

    Msr::write (Msr::IA32_EFER, Msr::read<uint32>(Msr::IA32_EFER) | EFER_SVME);
    Msr::write (Msr::AMD_SVM_HSAVE_PA, root = Kmem::ptr_to_phys (new Vmcb));

    trace (TRACE_VIRT, "VMCB:%#010lx REV:%#x NPT:%d", root, svm_version, has_npt());
}
