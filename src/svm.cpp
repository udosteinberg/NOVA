/*
 * Secure Virtual Machine (SVM)
 *
 * Author: Udo Steinberg <udo@hypervisor.org>
 * TU Dresden, Operating Systems Group
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
uint32              Vmcb::svm_version;
uint32              Vmcb::svm_feature;

Vmcb::Vmcb (mword bmp, mword nptp) : base_io (bmp), asid (++asid_ctr), int_control (1ul << 24), npt_control (1), npt_cr3 (nptp), efer (Cpu::EFER_SVME), g_pat (0x7040600070406ull)
{
    base_msr = Buddy::ptr_to_phys (Buddy::allocator.alloc (1, Buddy::FILL_1));
}

void Vmcb::init()
{
    if (!Cpu::feature (Cpu::FEAT_SVM) || !(Vmcb::svm_feature & 1)) {
        Hip::remove (Hip::FEAT_SVM);
        return;
    }

    Msr::write (Msr::IA32_EFER, Msr::read<uint32>(Msr::IA32_EFER) | Cpu::EFER_SVME);
    Msr::write (Msr::AMD_SVM_HSAVE_PA, Buddy::ptr_to_phys (new Vmcb));

    trace (TRACE_SVM, "VMCB:%#010lx REV:%#x FEAT:%#x", Buddy::ptr_to_phys (root), svm_version, svm_feature);
}
