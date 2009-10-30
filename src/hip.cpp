/*
 * Hypervisor Information Page (HIP)
 *
 * Copyright (C) 2008-2009, Udo Steinberg <udo@hypervisor.org>
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

#include "cpu.h"
#include "hip.h"
#include "lapic.h"
#include "multiboot.h"
#include "ptab.h"
#include "space_obj.h"

mword Hip::root_addr;
mword Hip::root_size;

void Hip::build (mword addr)
{
    Hip *h = hip();

    h->signature  = 0x41564f4e;
    h->cpu_offs   = reinterpret_cast<mword>(h->cpu_desc) - reinterpret_cast<mword>(h);
    h->cpu_size   = sizeof (Hip_cpu);
    h->mem_offs   = reinterpret_cast<mword>(h->mem_desc) - reinterpret_cast<mword>(h);
    h->mem_size   = sizeof (Hip_mem);
    h->api_flg    = FEAT_VMX | FEAT_SVM;
    h->api_ver    = 0x1;
    h->cfg_cap    = Space_obj::caps;
    h->cfg_pre    = NUM_PRE;
    h->cfg_gsi    = NUM_GSI;
    h->cfg_page   = PAGE_SIZE;
    h->cfg_utcb   = PAGE_SIZE;

    Multiboot *mbi = static_cast<Multiboot *>(Ptab::master()->remap (addr));

    uint32 flags       = mbi->flags;
    uint32 mmap_addr   = mbi->mmap_addr;
    uint32 mmap_len    = mbi->mmap_len;
    uint32 mods_addr   = mbi->mods_addr;
    uint32 mods_count  = mbi->mods_count;

    Hip_mem *mem = h->mem_desc;

    if (flags & Multiboot::MEMORY_MAP)
        add_mem (mem, mmap_addr, mmap_len);

    if (flags & Multiboot::MODULES)
        add_mod (mem, mods_addr, mods_count);

    add_mhv (mem);

    h->length = static_cast<uint16>(reinterpret_cast<mword>(mem) - reinterpret_cast<mword>(h));
}

void Hip::add_mem (Hip_mem *&mem, mword addr, size_t len)
{
    char *mmap_addr = static_cast<char *>(Ptab::master()->remap (addr));

    for (char *ptr = mmap_addr; ptr < mmap_addr + len; mem++) {

        Multiboot_mmap *map = reinterpret_cast<Multiboot_mmap *>(ptr);

        mem->addr = map->addr;
        mem->size = map->len;
        mem->type = map->type;
        mem->aux  = 0;

        ptr += map->size + 4;
    }
}

void Hip::add_mod (Hip_mem *&mem, mword addr, size_t count)
{
    Multiboot_module *mod = static_cast<Multiboot_module *>(Ptab::master()->remap (addr));

    if (count) {
        root_addr = mod->s_addr;
        root_size = mod->e_addr - mod->s_addr;
    }

    for (unsigned i = 0; i < count; i++, mod++, mem++) {

        mem->addr = mod->s_addr;
        mem->size = mod->e_addr - mod->s_addr;
        mem->type = Hip_mem::MB_MODULE;
        mem->aux  = mod->cmdline;
    }
}

void Hip::add_mhv (Hip_mem *&mem)
{
    mem->addr = LOAD_ADDR;
    mem->size = reinterpret_cast<mword>(&LOAD_SIZE);
    mem->type = Hip_mem::HYPERVISOR;
    mem++;

    mem->addr = reinterpret_cast<mword>(&LINK_PHYS);
    mem->size = reinterpret_cast<mword>(&LINK_SIZE);
    mem->type = Hip_mem::HYPERVISOR;
    mem++;
}

void Hip::add_cpu()
{
    Hip_cpu *cpu = hip()->cpu_desc + Cpu::id;

    cpu->package = static_cast<uint8>(Cpu::package);
    cpu->core    = static_cast<uint8>(Cpu::core);
    cpu->thread  = static_cast<uint8>(Cpu::thread);
    cpu->flags   = 1;
}

void Hip::add_check()
{
    Hip *h = hip();

    h->freq_tsc = Lapic::freq_tsc;
    h->freq_bus = Lapic::freq_bus;

    uint16 c = 0;
    for (uint16 const *ptr = reinterpret_cast<uint16 const *>(&PAGE_H);
                       ptr < reinterpret_cast<uint16 const *>(&PAGE_H + h->length);
                       c = static_cast<uint16>(c - *ptr++)) ;

    h->checksum = c;
}
