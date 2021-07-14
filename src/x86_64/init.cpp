/*
 * Initialization Code
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "acpi.hpp"
#include "cmdline.hpp"
#include "compiler.hpp"
#include "extern.hpp"
#include "ioapic.hpp"
#include "interrupt.hpp"
#include "kmem.hpp"
#include "pic.hpp"
#include "stdio.hpp"
#include "string.hpp"

extern "C"
Hpt::OAddr kern_ptab_setup()
{
    Hptp hptp;

    // Sync kernel code and data
    hptp.sync_from_master (LINK_ADDR, MMAP_CPU);

    // Allocate and map cpu page
    hptp.update (MMAP_CPU_DATA, Kmem::ptr_to_phys (Buddy::alloc (0, Buddy::Fill::BITS0)), 0,
                 Paging::Permissions (Paging::G | Paging::W | Paging::R),
                 Memattr::Cacheability::MEM_WB, Memattr::Shareability::NONE);

    // Allocate and map kernel stack
    hptp.update (MMAP_CPU_STCK, Kmem::ptr_to_phys (Buddy::alloc (0, Buddy::Fill::BITS0)), 0,
                 Paging::Permissions (Paging::G | Paging::W | Paging::R),
                 Memattr::Cacheability::MEM_WB, Memattr::Shareability::NONE);

    return hptp.root_addr();
}

extern "C"
void init (uintptr_t offset)
{
    if (!Acpi::resume) {

        Kmem::init (offset);

        Buddy::init();

        // Setup 0-page and 1-page
        memset (reinterpret_cast<void *>(&PAGE_0),  0,  PAGE_SIZE);
        memset (reinterpret_cast<void *>(&PAGE_1), ~0u, PAGE_SIZE);

        for (void (**func)() = &CTORS_G; func != &CTORS_E; (*func++)()) ;

        auto addr = *reinterpret_cast<uintptr_t *>(Kmem::sym_to_virt (&__boot_cl));
        if (addr)
            Cmdline::parse (static_cast<char const *>(Hptp::map (addr)));

        for (void (**func)() = &CTORS_C; func != &CTORS_G; (*func++)()) ;

        // Now we're ready to talk to the world
        Console::print ("\fNOVA Microhypervisor v%d-%07lx (%s): %s %s [%s]\n", CFG_VER, reinterpret_cast<uintptr_t>(&GIT_VER), ARCH, __DATE__, __TIME__, COMPILER_STRING);

        Interrupt::setup();
    }

    Acpi::init();

    Pic::init();

    Ioapic::init_all();

    Interrupt::init_all();
}
