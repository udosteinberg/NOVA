/*
 * Initialization Code
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

#include "config.hpp"
#include "console.hpp"
#include "cpu.hpp"
#include "extern.hpp"
#include "fdt.hpp"
#include "hpt.hpp"
#include "psci.hpp"

extern "C"
void kern_ptab_setup (unsigned cpu)
{
    Hptp hptp; mword phys;

    // Sync kernel code and data
    hptp.sync_from_master (LINK_ADDR, CPU_LOCAL);

    // Allocate and map kernel stack
    hptp.update (CPU_LOCAL_STCK, Buddy::ptr_to_phys (Buddy::allocator.alloc (0, Buddy::FILL_0)), 0,
                 Paging::Permissions (Paging::R | Paging::W | Paging::G),
                 Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);

    // Allocate and map cpu page
    hptp.update (CPU_LOCAL_DATA, phys = Buddy::ptr_to_phys (Buddy::allocator.alloc (0, Buddy::FILL_0)), 0,
                 Paging::Permissions (Paging::R | Paging::W | Paging::G),
                 Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);

    // Add to CPU array
    Hptp::master.update (CPU_GLOBL_DATA + cpu * PAGE_SIZE, phys, 0,
                         Paging::Permissions (Paging::R | Paging::W | Paging::G),
                         Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);

    hptp.make_current();
}

inline mword version()
{
    mword v = reinterpret_cast<mword>(GIT_VER + LINK_ADDR);

    return v - LINK_ADDR;
}

extern "C"
unsigned init()
{
    for (void (**func)() = &CTORS_G; func != &CTORS_E; (*func++)()) ;

    for (void (**func)() = &CTORS_C; func != &CTORS_G; (*func++)()) ;

    // Now we're ready to talk to the world
    Console::print ("\nNOVA Microhypervisor v%d-%07lx (%s): %s %s [%s]\n", CFG_VER, version(), ARCH, __DATE__, __TIME__, COMPILER_STRING);

    Fdt::init();

    Psci::init();

    return Cpu::boot_cpu;
}
