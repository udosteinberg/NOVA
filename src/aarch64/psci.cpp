/*
 * Power State Coordination Interface (PSCI)
 *
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "buddy.hpp"
#include "extern.hpp"
#include "hpt.hpp"
#include "psci.hpp"
#include "stdio.hpp"

void Psci::init()
{
    if (SPIN_ADDR) {

        auto spintable = reinterpret_cast<uint64 *>(Hpt::map (SPIN_ADDR, true));

        for (unsigned i = 0; i < CL0_CORES; i++) {
            spintable[i] = Buddy::sym_to_phys (__init_spin);
            asm volatile ("dsb ish; dc cvac, %0; sev" : : "r" (spintable + i) : "memory");
        }

        Cpu::online = CL0_CORES;
        return;
    }

    uint32 v = version();

    trace (TRACE_FIRM, "PSCI: Version %u.%u", v >> 16, v & 0xffff);

    Cpu::online = 1;

    // Brute-force boot cluster 0
    for (unsigned i = 0; i < CL0_CORES; i++) {
        if (cpu_on (i, Buddy::sym_to_phys (__init_psci), Cpu::online) != Status::SUCCESS)
            continue;
        trace (TRACE_FIRM, "PSCI: Booting CPU%u (%#x)", Cpu::online, i);
        Cpu::online++;
    }

    // Brute-force boot cluster 1
    for (unsigned i = 0x100; i < CL1_CORES + 0x100; i++) {
        if (cpu_on (i, Buddy::sym_to_phys (__init_psci), Cpu::online) != Status::SUCCESS)
            continue;
        trace (TRACE_FIRM, "PSCI: Booting CPU%u (%#x)", Cpu::online, i);
        Cpu::online++;
    }
}

void Psci::fini()
{
    trace (TRACE_FIRM, "Resetting...\n\n");
    system_reset();
}
