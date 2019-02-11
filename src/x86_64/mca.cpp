/*
 * Machine-Check Architecture (MCA)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#include "compiler.hpp"
#include "cpu.hpp"
#include "lowlevel.hpp"
#include "mca.hpp"
#include "msr.hpp"
#include "stdio.hpp"

unsigned Mca::banks;

void Mca::init()
{
    if (EXPECT_FALSE (!Cpu::feature (Cpu::FEAT_MCE)))
        return;

    set_cr4 (get_cr4() | Cpu::CR4_MCE);

    if (EXPECT_FALSE (!Cpu::feature (Cpu::FEAT_MCA)))
        return;

    uint32 cap = static_cast<uint32>(Msr::read (Msr::IA32_MCG_CAP));

    Msr::write (Msr::IA32_MCG_STATUS, 0);

    if (cap & 0x100)
        Msr::write (Msr::IA32_MCG_CTL, ~0ULL);

    banks = cap & 0xff;

    for (unsigned i = (Cpu::vendor == Cpu::INTEL && Cpu::family == 6 && Cpu::model < 0x1a); i < banks; i++) {
        Msr::write (Msr::Register (4 * i + Msr::IA32_MCI_CTL), ~0ULL);
        Msr::write (Msr::Register (4 * i + Msr::IA32_MCI_STATUS), 0);
    }
}

void Mca::vector()
{
    uint64 sts;

    for (unsigned i = 0; i < banks; i++)
        if ((sts = Msr::read (Msr::Register (4 * i + Msr::IA32_MCI_STATUS))) & 1ULL << 63)
            trace (TRACE_ERROR, "Machine Check B%u: %#018llx", i, sts);
}
