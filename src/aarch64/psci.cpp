/*
 * Power State Coordination Interface (PSCI)
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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
#include "hpt.hpp"
#include "psci.hpp"
#include "stdio.hpp"

bool Psci::boot_cpu (unsigned id, uint64 mpidr)
{
    if (cpu_on (mpidr, Kmem::sym_to_phys (__init_psci), id) != Status::SUCCESS)
        return false;

    trace (TRACE_FIRM, "PSCI: Booting CPU%u (%#llx)", id, mpidr);

    return true;
}

void Psci::init()
{
    unsigned cnt = sizeof (Board::cpu) / sizeof (*Board::cpu);

    if (Board::spin_addr) {

        auto spintable = reinterpret_cast<uint64 *>(Hpt::map (Board::spin_addr, true));

        for (unsigned i = 0; i < cnt; i++) {
            spintable[i] = Kmem::sym_to_phys (__init_spin);
            asm volatile ("dsb ish; dc cvac, %0; sev" : : "r" (spintable + i) : "memory");
        }

        Cpu::online = cnt;
        return;
    }

    uint32 v = version();

    trace (TRACE_FIRM, "PSCI: Version %u.%u", v >> 16, v & 0xffff);

    Cpu::online = 1;

    for (unsigned i = 0; i < cnt; i++)
        if (boot_cpu (Cpu::online, Board::cpu[i].id))
            Cpu::online++;
}

void Psci::fini()
{
    trace (TRACE_FIRM, "Resetting...\n\n");
    system_reset();
}
