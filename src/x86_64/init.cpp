/*
 * Initialization Code
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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
#include "cmdline.hpp"
#include "compiler.hpp"
#include "extern.hpp"
#include "hpt.hpp"
#include "idt.hpp"
#include "interrupt.hpp"
#include "kmem.hpp"
#include "stdio.hpp"
#include "string.hpp"

extern "C"
Hpt::OAddr kern_ptab_setup()
{
    Hptp hptp;

    // Sync kernel code and data
    hptp.sync_master_range (LINK_ADDR, CPU_LOCAL);

    // Allocate and map cpu page
    hptp.update (CPU_LOCAL_DATA, Kmem::ptr_to_phys (Buddy::alloc (0, Buddy::Fill::BITS0)), 0,
                 Paging::Permissions (Paging::R | Paging::W | Paging::G),
                 Memattr::Cacheability::MEM_WB, Memattr::Shareability::NONE);

    // Allocate and map kernel stack
    hptp.update (CPU_LOCAL_STCK, Kmem::ptr_to_phys (Buddy::alloc (0, Buddy::Fill::BITS0)), 0,
                 Paging::Permissions (Paging::R | Paging::W | Paging::G),
                 Memattr::Cacheability::MEM_WB, Memattr::Shareability::NONE);

    return hptp.init_root (false);
}

extern "C"
void init (uintptr_t offset)
{
    Kmem::init (offset);

    Buddy::init();

    // Setup 0-page and 1-page
    memset (reinterpret_cast<void *>(&PAGE_0),  0,  PAGE_SIZE);
    memset (reinterpret_cast<void *>(&PAGE_1), ~0u, PAGE_SIZE);

    for (void (**func)() = &CTORS_G; func != &CTORS_E; (*func++)()) ;

    auto addr = *reinterpret_cast<uintptr_t *>(Kmem::sym_to_virt (&__boot_cl));
    if (addr)
        Cmdline::parse (static_cast<char const *>(Hpt::map (addr)));

    for (void (**func)() = &CTORS_C; func != &CTORS_G; (*func++)()) ;

    // Now we're ready to talk to the world
    Console::print ("\fNOVA Microhypervisor v%d-%07lx (%s): %s %s [%s]\n", CFG_VER, reinterpret_cast<mword>(&GIT_VER), ARCH, __DATE__, __TIME__, COMPILER_STRING);

    Idt::build();
    Interrupt::init();
    Acpi::init();
}
