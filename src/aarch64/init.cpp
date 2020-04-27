/*
 * Initialization Code
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

#include "acpi.hpp"
#include "buddy.hpp"
#include "config.hpp"
#include "console.hpp"
#include "cpu.hpp"
#include "extern.hpp"
#include "fdt.hpp"
#include "hpt.hpp"
#include "kmem.hpp"

extern "C"
Hpt::OAddr kern_ptab_setup (unsigned cpu)
{
    Hptp hptp; uintptr_t phys;

    // Sync kernel code and data
    hptp.sync_from_master (LINK_ADDR, CPU_LOCAL);

    // Allocate and map kernel stack
    hptp.update (CPU_LOCAL_STCK, Kmem::ptr_to_phys (Buddy::alloc (0, Buddy::Fill::BITS0)), 0,
                 Paging::Permissions (Paging::G | Paging::W | Paging::R),
                 Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);

    // Allocate and map cpu page
    hptp.update (CPU_LOCAL_DATA, phys = Kmem::ptr_to_phys (Buddy::alloc (0, Buddy::Fill::BITS0)), 0,
                 Paging::Permissions (Paging::G | Paging::W | Paging::R),
                 Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);

    // Add to CPU array
    Hptp::master.update (CPU_GLOBL_DATA + cpu * PAGE_SIZE, phys, 0,
                         Paging::Permissions (Paging::G | Paging::W | Paging::R),
                         Memattr::Cacheability::MEM_WB, Memattr::Shareability::INNER);

    return hptp.init_root (false);
}

extern "C"
unsigned init (uintptr_t offset)
{
    Kmem::init (offset);

    Buddy::init();

    for (void (**func)() = &CTORS_G; func != &CTORS_E; (*func++)()) ;

    for (void (**func)() = &CTORS_C; func != &CTORS_G; (*func++)()) ;

    // Now we're ready to talk to the world
    Console::print ("\nNOVA Microhypervisor v%d-%07lx (%s): %s %s [%s]\n", CFG_VER, reinterpret_cast<uintptr_t>(GIT_VER), ARCH, __DATE__, __TIME__, COMPILER_STRING);

    Acpi::init() || Fdt::init();

    return Cpu::boot_cpu;
}
