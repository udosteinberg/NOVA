/*
 * Machine-Check Architecture (MCA)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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
#include "stdio.hpp"

void Mca::init()
{
    // Check if MCA is available
    if (!Cpu::feature (Cpu::Feature::MCA)) [[unlikely]]
        return;

    auto const cap { Msr::read (Msr::Reg64::IA32_MCG_CAP) };

    // Determine number of reporting banks
    banks = cap & BIT_RANGE (7, 0);

    // Enable all MCA features
    if (cap & BIT (8)) [[likely]]
        Msr::write (Msr::Reg64::IA32_MCG_CTL, BIT64_RANGE (63, 0));

    unsigned cmci { 0 };

    // Enable CMCI if supported and when error count reaches threshold 1
    if (cap & BIT (10)) [[likely]]
        for (unsigned i { 0 }; i < banks; i++) {
            Msr::write (Msr::Arr64::IA32_MC_CTL2, 1, i, BIT (30) | 1);
            if (Msr::read (Msr::Arr64::IA32_MC_CTL2, 1, i) & BIT (30))
                cmci++;
        }

    // Enable LMCE if supported
    if (cap & BIT (27)) [[likely]] {
        constexpr auto ctrl { BIT (20) | BIT (0) };     // LMCE On + LOCK
        if ((Msr::read (Msr::Reg64::IA32_FEATURE_CONTROL) & ctrl) == ctrl)
            Msr::write (Msr::Reg64::IA32_MCG_EXT_CTL, Msr::read (Msr::Reg64::IA32_MCG_EXT_CTL) | BIT (0));
    }

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
    for (unsigned i { Cpu::vendor == Cpu::Vendor::INTEL && Cpu::family == 6 && Cpu::model < 0x1a }; i < banks; i++) {
        Msr::write (Msr::Arr64::IA32_MC_CTL, 4, i, BIT64_RANGE (63, 0));
        Msr::write (Msr::Arr64::IA32_MC_STATUS, 4, i, 0);
    }

    Msr::write (Msr::Reg64::IA32_MCG_STATUS, 0);

    trace (TRACE_MCA, "MCHK: %u/%u banks", cmci, banks);
}

void Mca::handler()
{
    // Fatal if RIPV is 0
    bool fatal { !(Msr::read (Msr::Reg64::IA32_MCG_STATUS) & BIT (0)) };

    // Check all banks
    for (unsigned i { 0 }; i < banks; i++) {

        auto const s { Msr::read (Msr::Arr64::IA32_MC_STATUS, 4, i) };

        // Check for valid error
        if (!(s & BIT64 (63)))
            continue;

        trace (TRACE_MCA, "MCHK: %#04x ERR:%#06x MSC:%#06x OVER:%u UC:%u PCC:%u", i, static_cast<uint16_t>(s), static_cast<uint16_t>(s >> 16), !!(s & BIT64 (62)), !!(s & BIT64 (61)), !!(s & BIT64 (57)));

        // Fatal if OVER or PCC are 1
        fatal |= s & (BIT64 (62) | BIT64 (57));

        // Clear error
        Msr::write (Msr::Arr64::IA32_MC_STATUS, 4, i, 0);
    }

    if (fatal)
        panic ("Machine-check recovery impossible");

    // Clear MCIP
    Msr::write (Msr::Reg64::IA32_MCG_STATUS, 0);
}
