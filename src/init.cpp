/*
 * Initialization Code
 *
 * Copyright (C) 2005-2009, Udo Steinberg <udo@hypervisor.org>
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

#include "acpi.h"
#include "buddy.h"
#include "cmdline.h"
#include "compiler.h"
#include "extern.h"
#include "gsi.h"
#include "hip.h"
#include "idt.h"
#include "keyb.h"
#include "memory.h"
#include "multiboot.h"
#include "pd.h"
#include "pic.h"
#include "ptab.h"
#include "ptab_boot.h"
#include "stdio.h"
#include "types.h"

char const *version = "NOVA 0.1 (Xmas Alpha)";

extern "C" INIT
mword kern_ptab_setup()
{
    Ptab *ptab = new Ptab;

    // Allocate and map cpu page
    ptab->insert (CPULC_ADDR, 0,
                  Ptab::Attribute (Ptab::ATTR_NOEXEC |
                                   Ptab::ATTR_GLOBAL |
                                   Ptab::ATTR_WRITABLE),
                  Buddy::ptr_to_phys (Buddy::allocator.alloc (0, Buddy::FILL_0)));

    // Allocate and map kernel stack
    ptab->insert (KSTCK_ADDR, 0,
                  Ptab::Attribute (Ptab::ATTR_NOEXEC |
                                   Ptab::ATTR_GLOBAL |
                                   Ptab::ATTR_WRITABLE),
                  Buddy::ptr_to_phys (Buddy::allocator.alloc (0, Buddy::FILL_0)));

    // Sync kernel code and data
    ptab->sync_master_range (LINK_ADDR, LOCAL_SADDR);

    return Buddy::ptr_to_phys (ptab);
}

extern "C" INIT REGPARM (1)
void init_ilp (mword mbi)
{
    Multiboot *mb = reinterpret_cast<Multiboot *>(mbi);

    // Parse command line
    if (mb && mb->flags & Multiboot::CMDLINE)
        Cmdline::init (reinterpret_cast<char *>(mb->cmdline));

    // Initialize boot page table
    Ptab_boot::init();

    // Enable paging
    Paging::enable();

    // Call static constructors
    for (void (**func)() = &CTORS_E; func != &CTORS_S; (*--func)()) ;

    // Setup 0-page and 1-page
    memset (reinterpret_cast<void *>(&PAGE_0),  0,  PAGE_SIZE);
    memset (reinterpret_cast<void *>(&PAGE_1), ~0u, PAGE_SIZE);

    // Setup consoles
    serial.init();
    screen.init();

     // Now we're ready to talk to the world
    printf ("\f%s: %s %s [%s]\n\n", version, __DATE__, __TIME__, COMPILER);

    Hip::build (mbi);

    // Initialize 8259A interrupt controllers
    Pic::master.init (VEC_GSI);
    Pic::slave.init  (VEC_GSI + 8);

    Idt::build();
    Gsi::setup();
    Acpi::setup();

    Keyb::init();
}

extern "C" INIT
void init_rlp()
{
    // Enable paging
    Paging::enable();
}
