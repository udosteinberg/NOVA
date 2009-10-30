/*
 * Safer Mode Extensions (SMX)
 *
 * Copyright (C) 2007-2008, Udo Steinberg <udo@hypervisor.org>
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

#include "cmdline.h"
#include "cpu.h"
#include "extern.h"
#include "mtrr.h"
#include "pd.h"
#include "smx.h"
#include "stdio.h"

void Smx::launch (Multiboot *mbi)
{
    // Parse command line
    if (mbi && mbi->flags & Multiboot::CMDLINE)
        Cmdline::init (reinterpret_cast<char *>(mbi->cmdline));

    // SMX disable check
    if (*static_cast<bool *>(Cmdline::phys_ptr (&Cmdline::nosmx)))
        return;

    // SMX feature check
    uint32 dummy, ecx;
    Cpu::cpuid (1, dummy, dummy, ecx, dummy);
    if (!(ecx & 1ul << 6))
        return;

    // Enable SMX
    Cpu::set_cr4 (Cpu::get_cr4() | Cpu::CR4_SMXE);

    // SMX capability check
    uint32 cap;
    capabilities (0, cap);
    if ((cap & cap_req) != cap_req)
        return;

    uint32 txt_sinit_base = phys_read<uint32>(TXT_SINIT_BASE);
    uint32 txt_sinit_size = phys_read<uint32>(TXT_SINIT_SIZE);
    uint32 txt_heap_base  = phys_read<uint32>(TXT_HEAP_BASE);
    uint32 txt_heap_size  = phys_read<uint32>(TXT_HEAP_SIZE);
    uint32 txt_didvid     = phys_read<uint32>(TXT_DIDVID);

    if (!txt_sinit_size || !txt_heap_size)
        return;

    AC_Module *mod = reinterpret_cast<AC_Module *>(txt_sinit_base);
    txt_sinit_size = mod->size * 4;

    // BIOS to OS data
    BIOS_OS *bios_os      = reinterpret_cast<BIOS_OS *>(txt_heap_base);
    txt_heap_base         += static_cast<unsigned>(bios_os->size);

    if (!bios_os->sinit_size)
        return;

    // OS to MLE data
    OS_MLE *os_mle        = reinterpret_cast<OS_MLE *>(txt_heap_base);
    txt_heap_base        += static_cast<uint32>(sizeof (OS_MLE));
    os_mle->size          = sizeof (OS_MLE);
    os_mle->mbi           = reinterpret_cast<mword>(mbi);
    os_mle->mtrr.save (txt_sinit_base, txt_sinit_size);

    // Setup MLE page tables
    PDP = reinterpret_cast<mword>(&PDE) | 1;
    PDE = reinterpret_cast<mword>(&PTE) | 1;
    uint64 *pte = &PTE;

    extern char INIT_SIZE, HASH_SIZE, LINK_PHYS;
    size_t boot_size = reinterpret_cast<mword>(&INIT_SIZE);
    size_t hash_size = reinterpret_cast<mword>(&HASH_SIZE);
    mword  link_phys = reinterpret_cast<mword>(&LINK_PHYS);

    for (mword addr = INIT_ADDR; addr < INIT_ADDR + boot_size; addr += PAGE_SIZE)
        *pte++ = addr | 1;

    for (mword addr = link_phys; addr < link_phys + hash_size; addr += PAGE_SIZE)
        *pte++ = addr | 1;

    // OS to SINIT data
    extern char __mle_header, LINK_PHYS, LINK_SIZE;
    OS_SINIT *os_sinit    = reinterpret_cast<OS_SINIT *>(txt_heap_base);
    os_sinit->size        = sizeof (OS_SINIT);
    os_sinit->version     = txt_didvid >> 16 == 0x8000 ? 1: 3;
    os_sinit->mle_ptab    = reinterpret_cast<mword>(&PDP);
    os_sinit->mle_size    = boot_size + hash_size;
    os_sinit->mle_header  = reinterpret_cast<mword>(&__mle_header) - INIT_ADDR;
    os_sinit->pmr_lo_base = LOAD_ADDR & ~0x1fffff;
    os_sinit->pmr_lo_size = reinterpret_cast<mword>(&LINK_PHYS) + reinterpret_cast<mword>(&LINK_SIZE);
    os_sinit->pmr_hi_base = 0;
    os_sinit->pmr_hi_size = 0;
    os_sinit->lcp_po_base = 0;
    os_sinit->lcp_po_size = 0;

    // Disable TPM locality 0
    *reinterpret_cast<uint8 *>(0xfed40000) = 1u << 5;

    Cpu::set_cr0 ((Cpu::get_cr0() | Cpu::CR0_NE | Cpu::CR0_PE) &
                                  ~(Cpu::CR0_CD | Cpu::CR0_NW));

    senter (txt_sinit_base, txt_sinit_size);
}
