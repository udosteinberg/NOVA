/*
 * Initialization Code
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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
#include "console_serial.hpp"
#include "gsi.hpp"
#include "hpt.hpp"
#include "idt.hpp"
#include "patch.hpp"
#include "pic.hpp"
#include "string.hpp"

extern "C" uintptr_t kern_ptab_setup (apic_t)
{
    Hptp hpt;

    // Allocate and map cpu page
    hpt.update (MMAP_CPU_DATA, 0,
                Kmem::ptr_to_phys (Buddy::allocator.alloc (0, Buddy::FILL_0)),
                Hpt::HPT_NX | Hpt::HPT_G | Hpt::HPT_W | Hpt::HPT_P);

    // Allocate and map kernel stack
    hpt.update (MMAP_CPU_DSTK, 0,
                Kmem::ptr_to_phys (Buddy::allocator.alloc (0, Buddy::FILL_0)),
                Hpt::HPT_NX | Hpt::HPT_G | Hpt::HPT_W | Hpt::HPT_P);

    // Sync kernel code and data
    hpt.sync_master_range (LINK_ADDR, MMAP_CPU);

    return hpt.addr();
}

extern "C" void preinit()
{
    Patch::detect();
}

extern "C" void init()
{
    Patch::init();

    // Setup 0-page and 1-page
    memset (reinterpret_cast<void *>(&PAGE_0),  0,  PAGE_SIZE (0));
    memset (reinterpret_cast<void *>(&PAGE_1), ~0u, PAGE_SIZE (0));

    for (void (**func)() = &CTORS_S; func != &CTORS_E; (*func++)()) ;

    Cmdline::init();

    for (void (**func)() = &CTORS_C; func != &CTORS_S; (*func++)()) ;

    // Now we're ready to talk to the world
    Console::print ("\fNOVA Microhypervisor v%d-%07lx (%s): %s %s [%s]\n", CFG_VER, reinterpret_cast<mword>(&GIT_VER), ARCH, __DATE__, __TIME__, COMPILER_STRING);

    Idt::build();
    Gsi::setup();
    Acpi::setup();

    Pic::init();
}
