/*
 * Hypervisor Information Page (HIP)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "cmdline.hpp"
#include "cpu.hpp"
#include "hip.hpp"
#include "lapic.hpp"
#include "ptab_hpt.hpp"
#include "space_obj.hpp"

mword Hip::root_addr;
mword Hip::root_size;

Hip *Hip::hip = reinterpret_cast<Hip *>(&PAGE_H);

void Hip::build (mword)
{
    signature  = 0x41564f4e;
    cpu_offs   = reinterpret_cast<mword>(cpu_desc) - reinterpret_cast<mword>(this);
    cpu_size   = static_cast<uint16>(sizeof (Hip_cpu));
    mem_offs   = reinterpret_cast<mword>(mem_desc) - reinterpret_cast<mword>(this);
    mem_size   = static_cast<uint16>(sizeof (Hip_mem));
    api_flg    = FEAT_VMX | FEAT_SVM;
    api_ver    = CFG_VER;
    sel_num    = Space_obj::caps;
    sel_gsi    = NUM_GSI;
    sel_exc    = NUM_EXC;
    sel_vmi    = NUM_VMI;
    cfg_page   = PAGE_SIZE;
    cfg_utcb   = PAGE_SIZE;

    length = static_cast<uint16>(reinterpret_cast<mword>(mem_desc) - reinterpret_cast<mword>(this));
}

void Hip::add_mhv (Hip_mem *&mem)
{
    mem->addr = LOAD_ADDR;
    mem->size = reinterpret_cast<mword>(&NOVA_HPAE) - mem->addr;
    mem->type = Hip_mem::HYPERVISOR;
    mem++;
}

void Hip::add_check()
{
    freq_tsc = Lapic::freq_tsc;

    uint16 c = 0;
    for (uint16 const *ptr = reinterpret_cast<uint16 const *>(this);
                       ptr < reinterpret_cast<uint16 const *>(reinterpret_cast<mword>(this + length));
                       c = static_cast<uint16>(c - *ptr++)) ;

    checksum = c;
}
