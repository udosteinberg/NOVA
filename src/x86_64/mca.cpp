/*
 * Machine-Check Architecture (MCA)
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

#include "compiler.hpp"
#include "cpu.hpp"
#include "mca.hpp"
#include "msr.hpp"
#include "stdio.hpp"

unsigned Mca::banks;

void Mca::init()
{
    if (EXPECT_FALSE (!Cpu::feature (Cpu::Feature::MCA)))
        return;

    uint32 cap = static_cast<uint32>(Msr::read (Msr::Register::IA32_MCG_CAP));

    Msr::write (Msr::Register::IA32_MCG_STATUS, 0);

    if (cap & 0x100)
        Msr::write (Msr::Register::IA32_MCG_CTL, ~0ULL);

    banks = cap & 0xff;

    for (unsigned i = (Cpu::vendor == Cpu::Vendor::INTEL && Cpu::family == 6 && Cpu::model < 0x1a); i < banks; i++) {
        Msr::write (Msr::Array::IA32_MC_CTL, 4, i, ~0ULL);
        Msr::write (Msr::Array::IA32_MC_STATUS, 4, i, 0);
    }
}

void Mca::vector()
{
    uint64 sts;

    for (unsigned i = 0; i < banks; i++)
        if ((sts = Msr::read (Msr::Array::IA32_MC_STATUS, 4, i)) & 1ULL << 63)
            trace (TRACE_ERROR, "Machine Check B%u: %#018llx", i, sts);
}
