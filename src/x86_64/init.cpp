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
#include "hpt.hpp"
#include "ioapic.hpp"
#include "interrupt.hpp"
#include "kmem.hpp"
#include "pic.hpp"
#include "stdio.hpp"
#include "string.hpp"

extern "C"
mword kern_ptab_setup()
{
    Hptp hpt;

    // Allocate and map cpu page
    hpt.update (MMAP_CPU_DATA, 0,
                Kmem::ptr_to_phys (Buddy::allocator.alloc (0, Buddy::FILL_0)),
                Hpt::HPT_NX | Hpt::HPT_G | Hpt::HPT_W | Hpt::HPT_P);

    // Allocate and map kernel stack
    hpt.update (MMAP_CPU_STCK, 0,
                Kmem::ptr_to_phys (Buddy::allocator.alloc (0, Buddy::FILL_0)),
                Hpt::HPT_NX | Hpt::HPT_G | Hpt::HPT_W | Hpt::HPT_P);

    // Sync kernel code and data
    hpt.sync_master_range (LINK_ADDR, MMAP_CPU);

    return hpt.addr();
}

extern "C"
void init (uintptr_t offset)
{
    Kmem::init (offset);

    // Setup 0-page and 1-page
    memset (reinterpret_cast<void *>(&PAGE_0),  0,  PAGE_SIZE);
    memset (reinterpret_cast<void *>(&PAGE_1), ~0u, PAGE_SIZE);

    for (void (**func)() = &CTORS_G; func != &CTORS_E; (*func++)()) ;

    auto addr = *reinterpret_cast<uintptr_t *>(Kmem::sym_to_virt (&__boot_cl));
    if (addr)
        Cmdline::parse (static_cast<char const *>(Hpt::remap (addr)));

    for (void (**func)() = &CTORS_C; func != &CTORS_G; (*func++)()) ;

    // Now we're ready to talk to the world
    Console::print ("\fNOVA Microhypervisor v%d-%07lx (%s): %s %s [%s]\n", CFG_VER, reinterpret_cast<mword>(&GIT_VER), ARCH, __DATE__, __TIME__, COMPILER_STRING);

    Interrupt::setup();
    Acpi::setup();

    Pic::init();

    Ioapic::init_all();

    Interrupt::init_all();
}
