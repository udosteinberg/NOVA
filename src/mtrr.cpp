/*
 * Memory Type Range Registers (MTRR)
 *
 * Copyright (C) 2007-2009, Udo Steinberg <udo@hypervisor.org>
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

Mtrr::Type Mtrr::memtype (mword addr)
{
    if (addr < 0x80000)
        return static_cast<Type>(Msr::read<uint64>(Msr::IA32_MTRR_FIX64K_BASE) >>
                                (addr >> 13 & 0x38) & 0xff);
    if (addr < 0xc0000)
        return static_cast<Type>(Msr::read<uint64>(Msr::Register (Msr::IA32_MTRR_FIX16K_BASE + (addr >> 17 & 0x1))) >>
                                (addr >> 11 & 0x38) & 0xff);
    if (addr < 0x100000)
        return static_cast<Type>(Msr::read<uint64>(Msr::Register (Msr::IA32_MTRR_FIX4K_BASE  + (addr >> 15 & 0x7))) >>
                                (addr >>  9 & 0x38) & 0xff);

    return WB;
}
