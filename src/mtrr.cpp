/*
 * Memory Type Range Registers (MTRR)
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

#include "bits.h"
#include "msr.h"
#include "mtrr.h"
#include "x86.h"

void Mtrr::save (mword base, size_t size)
{
    // Page-align sinit size
    size = align_up (size, PAGE_SIZE);

    // Compute mask according to supported physical address bits
    uint64 mask = (((1ull << 36) - 1) & ~PAGE_MASK) | 1ul << 11;

    // XXX: Disable cache

    // Flush cache
    wbinvd();

    // Determine variable MTRR count
    count = Msr::read<uint32>(Msr::IA32_MTRR_CAP) & 0xff;
    dtype = Msr::read<uint32>(Msr::IA32_MTRR_DEF_TYPE);

    // Disable all MTRRs
    Msr::write (Msr::IA32_MTRR_DEF_TYPE, 0);

    // Setup variable MTRRs
    for (unsigned i = 0; i < count; i++) {

        mtrr[i].base = Msr::read<uint64>(Msr::Register (Msr::IA32_MTRR_PHYS_BASE + 2 * i));
        Msr::write (Msr::Register (Msr::IA32_MTRR_PHYS_BASE + 2 * i), base | WB);

        mtrr[i].mask = Msr::read<uint64>(Msr::Register (Msr::IA32_MTRR_PHYS_MASK + 2 * i));
        Msr::write (Msr::Register (Msr::IA32_MTRR_PHYS_MASK + 2 * i), size ? mask : 0);

        if (size) {
            size -= PAGE_SIZE;
            base += PAGE_SIZE;
        }
    }

    // Reenable variable MTRRs with default type UC
    Msr::write (Msr::IA32_MTRR_DEF_TYPE, 1ul << 11 | UC);

    // XXX: Enable cache
}

void Mtrr::load()
{
    // XXX: Disable cache

    // Setup variable MTRRs
    for (unsigned i = 0; i < count; i++) {
        Msr::write (Msr::Register (Msr::IA32_MTRR_PHYS_BASE + 2 * i), mtrr[i].base);
        Msr::write (Msr::Register (Msr::IA32_MTRR_PHYS_MASK + 2 * i), mtrr[i].mask);
    }

    // Reenable variable MTRRs
    Msr::write (Msr::IA32_MTRR_DEF_TYPE, dtype);

    // XXX: Enable cache
}

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
