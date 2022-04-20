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

#include "cpu.hpp"
#include "mca.hpp"
#include "msr.hpp"
#include "stdio.hpp"

uint8 Mca::banks { 0 };

void Mca::init()
{
    if (EXPECT_FALSE (!Cpu::feature (Cpu::Feature::MCA)))
        return;

    auto cap = Msr::read (Msr::Register::IA32_MCG_CAP);

    if (cap & BIT (8))
        Msr::write (Msr::Register::IA32_MCG_CTL, BIT64_RANGE (63, 0));

    banks = cap & BIT_RANGE (7, 0);

    /*
     * For P6 family processors and Core family processors before NHM:
     * The OS must not modify the contents of the IA32_MC0_CTL MSR. This
     * MSR is internally aliased to EBL_CR_POWERON (0x2a) and controls
     * platform-specific error handling features. Firmware is responsible
     * for the appropriate initialization of IA32_MC0_CTL.
     *
     * P6 family processors only allow the writing of all 1s or all 0s to
     * the IA32_MCi_CTL MSRs.
     */
    for (unsigned i = (Cpu::vendor == Cpu::Vendor::INTEL && Cpu::family == 6 && Cpu::model < 0x1a); i < banks; i++) {
        Msr::write (Msr::Array::IA32_MC_CTL, 4, i, BIT64_RANGE (63, 0));
        Msr::write (Msr::Array::IA32_MC_STATUS, 4, i, 0);
    }

    Msr::write (Msr::Register::IA32_MCG_STATUS, 0);
}

void Mca::handler()
{
    uint64 sts;

    for (unsigned i = 0; i < banks; i++)
        if ((sts = Msr::read (Msr::Array::IA32_MC_STATUS, 4, i)) & BIT64 (63))
            trace (TRACE_CPU, "Machine Check Bank%u: %#018llx", i, sts);
}
