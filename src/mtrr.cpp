/*
 * Memory Type Range Registers (MTRR)
 *
 * Author: Udo Steinberg <udo@hypervisor.org>
 * TU Dresden, Operating Systems Group
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

#include "msr.h"
#include "mtrr.h"
#include "x86.h"

Mtrr Mtrr::mtrr;

void Mtrr::init()
{
    mtrr.count = Msr::read<uint32>(Msr::IA32_MTRR_CAP) & 0xff;
    mtrr.dtype = Msr::read<uint32>(Msr::IA32_MTRR_DEF_TYPE) & 0xff;

    for (unsigned i = 0; i < mtrr.count; i++) {
        mtrr.var[i].base = Msr::read<uint64>(Msr::Register (Msr::IA32_MTRR_PHYS_BASE + 2 * i));
        mtrr.var[i].mask = Msr::read<uint64>(Msr::Register (Msr::IA32_MTRR_PHYS_MASK + 2 * i));
    }
}

unsigned Mtrr::memtype (Paddr phys)
{
    if (phys < 0x80000)
        return static_cast<unsigned>(Msr::read<uint64>(Msr::IA32_MTRR_FIX64K_BASE) >>
                                    (phys >> 13 & 0x38)) & 0xff;
    if (phys < 0xc0000)
        return static_cast<unsigned>(Msr::read<uint64>(Msr::Register (Msr::IA32_MTRR_FIX16K_BASE + (phys >> 17 & 0x1))) >>
                                    (phys >> 11 & 0x38)) & 0xff;
    if (phys < 0x100000)
        return static_cast<unsigned>(Msr::read<uint64>(Msr::Register (Msr::IA32_MTRR_FIX4K_BASE  + (phys >> 15 & 0x7))) >>
                                    (phys >>  9 & 0x38)) & 0xff;

    for (unsigned i = 0; i < mtrr.count; i++)
        if ((mtrr.var[i].mask & 0x800) && ((phys ^ mtrr.var[i].base) & mtrr.var[i].mask) >> 12 == 0)
            return static_cast<unsigned>(mtrr.var[i].base) & 0xff;

    return mtrr.dtype;
}
