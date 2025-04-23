/*
 * SMC ARCH Calls (Arm Architecture Calls)
 *
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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

#include "smc_arch.hpp"
#include "stdio.hpp"

void Smc_arch::init()
{
    auto const v { version() };

    if (v >= 0x10001) [[likely]] {      // SMCCC 1.1+

        // Functions to check
        uint32_t const func[] {
            function (Function32::WORKAROUND_1),
            function (Function32::WORKAROUND_2),
            function (Function32::WORKAROUND_3),
            function (Function32::WORKAROUND_4),
        };

        // Determine applicable workaround functions
        for (unsigned i { 0 }; i < sizeof (func) / sizeof (*func); i++)
            if (std::to_underlying (features (func[i])) >= 0) [[likely]]
                workarounds |= BIT (i);
    }

    trace (TRACE_FIRM, "ARCH: Version %u.%u Errata %#x", v >> 16, v & BIT_RANGE (15, 0), workarounds);
}
