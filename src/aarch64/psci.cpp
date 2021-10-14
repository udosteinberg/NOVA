/*
 * Power State Coordination Interface (PSCI)
 *
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

#include "extern.hpp"
#include "kmem.hpp"
#include "psci.hpp"
#include "stdio.hpp"

bool Psci::boot_cpu (unsigned id, uint64 mpidr)
{
    auto const aff { Cpu::affinity_bits (mpidr) };

    switch (cpu_on (aff, Kmem::sym_to_phys (&__init_psci), id)) {

        case Status::ALREADY_ON:
            Cpu::boot_cpu = id;
            return true;

        case Status::SUCCESS:
            trace (TRACE_FIRM, "PSCI: Booting CPU%u (%#llx)", id, aff);
            return true;

        default:
            return false;
    }
}

void Psci::offline_wait()
{
}

void Psci::init()
{
    auto const ver { version() };

    if (ver >= 0x2)             // PSCI 0.2+
        states |= BIT (7) | BIT_RANGE (5, 4);

    if (ver >= 0x10000)         // PSCI 1.0+
        if (supported (Function64::SYSTEM_SUSPEND))
            states |= BIT_RANGE (3, 2);

    trace (TRACE_FIRM, "PSCI: Version %u.%u States:%#x", ver >> 16, ver & BIT_RANGE (15, 0), states);
}
