/*
 * Instruction Patching
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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

#include "fpu.hpp"
#include "lapic.hpp"
#include "memattr.hpp"
#include "patch.hpp"
#include "ptab_hpt.hpp"
#include "string.hpp"
#include "util.hpp"

void Patch::detect()
{
    uint32_t eax, ebx, ecx, edx;

    Cpu::cpuid (0x0, eax, ebx, ecx, edx);

    switch (static_cast<uint8_t>(eax)) {
        default:
            Cpu::cpuid (0xd, 0x1, eax, ebx, ecx, edx);
            skipped |= (Fpu::compact = !!(eax & BIT (3))) * BIT (PATCH_XSAVES);
            [[fallthrough]];
        case 0x7 ... 0xc:
            Cpu::cpuid (0x7, 0x0, eax, ebx, ecx, edx);
            if (ecx & BIT (13)) {
                auto const act { Msr::read (Msr::Reg64::IA32_TME_ACTIVATE) };
                if (act & BIT (1)) {
                    Memattr::crypt = act >> 48 & BIT_RANGE (2, 0);
                    Memattr::kbits = act >> 32 & BIT_RANGE (3, 0);
                    Memattr::kimax = min (BIT (Memattr::kbits) - 1, static_cast<unsigned>(Msr::read (Msr::Reg64::IA32_TME_CAPABILITY) >> 36 & BIT_RANGE (14, 0)));
                }
            }
            skipped |= !!(edx & BIT (20)) * BIT (PATCH_CET_IBT);
            Cpu::cpuid (0x7, 0x1, eax, ebx, ecx, edx);
            skipped |= !!(edx & BIT (18)) * BIT (PATCH_CET_SSS);
            [[fallthrough]];
        case 0x1 ... 0x6:
            Cpu::cpuid (0x1, 0x0, eax, ebx, ecx, edx);
            Lapic::x2apic = ecx & BIT (21);
            [[fallthrough]];
        case 0x0:
            break;
    }

    Cpu::cpuid (0x80000000, eax, ebx, ecx, edx);

    switch (static_cast<uint8_t>(eax)) {
        default:
            Cpu::cpuid (0x80000008, eax, ebx, ecx, edx);
            Memattr::obits = (BIT_RANGE (7, 0) & eax) - Memattr::kbits;
            [[fallthrough]];
        case 0x1 ... 0x7:
            Cpu::cpuid (0x80000001, eax, ebx, ecx, edx);
            Hptp::set_mll (edx & BIT (26) ? 2 : 1);
            [[fallthrough]];
        case 0x0:
            break;
    }
}

void Patch::init()
{
    extern Patch PATCH_S, PATCH_E;

    for (auto p { &PATCH_S }; p < &PATCH_E; p++) {

        if (skipped & BIT (p->tag))
            continue;

        auto const o { reinterpret_cast<uint8_t *>(p) + p->off_old };
        auto const n { reinterpret_cast<uint8_t *>(p) + p->off_new };

        memcpy (o, n, p->len_new);
        memset (o + p->len_new, NOP_OPC, p->len_old - p->len_new);
    }
}
