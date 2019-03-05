/*
 * Power State Coordination Interface (PSCI)
 *
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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
#include "psci.hpp"
#include "ptab_hpt.hpp"
#include "stdio.hpp"

bool Psci::boot_cpu (unsigned id, uint64 mpidr)
{
    switch (cpu_on (mpidr, Kmem::sym_to_phys (__init_psci), id)) {

        case Status::ALREADY_ON:
            Cpu::boot_cpu = id;
            return true;

        case Status::SUCCESS:
            trace (TRACE_FIRM, "PSCI: Booting CPU%u (%#llx)", id, mpidr);
            return true;

        default:
            return false;
    }
}

void Psci::init()
{
    unsigned cnt = sizeof (Board::cpu) / sizeof (*Board::cpu);

    if (Board::spin_addr) {

        auto spintable = reinterpret_cast<uint64 *>(Hptp::map (Board::spin_addr, true));

        for (unsigned i = 0; i < cnt; i++) {
            spintable[i] = Kmem::sym_to_phys (__init_spin);
            asm volatile ("dsb ish; dc cvac, %0; sev" : : "r" (spintable + i) : "memory");
        }

        Cpu::count = cnt;
        return;
    }

    uint32 v = version();

    trace (TRACE_FIRM, "PSCI: Version %u.%u", v >> 16, v & 0xffff);

    for (unsigned i = 0; i < cnt; i++)
        if (boot_cpu (i, Board::cpu[i].id))
            Cpu::count++;
}

void Psci::fini()
{
    trace (TRACE_FIRM, "Resetting...\n\n");
    system_reset();
}
