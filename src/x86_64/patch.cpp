/*
 * Instruction Patching
 *
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

void Patch::detect()
{
    uint32_t eax, ebx, ecx, edx;

    // Physical address bits reported by CPUID
    unsigned pbits { 0 };

    Cpu::cpuid (0x0, eax, ebx, ecx, edx);

    switch (static_cast<uint8_t>(eax)) {
        default:
            Cpu::cpuid (0xd, 0x1, eax, ebx, ecx, edx);
            Fpu::compact = !!(eax & BIT (3));
            applied |= BIT (PATCH_XSAVES) * !Fpu::compact;
            [[fallthrough]];
        case 0x7 ... 0xc:
            Cpu::cpuid (0x7, 0x0, eax, ebx, ecx, edx);
            applied |= BIT (PATCH_CET_IBT) * !(edx & BIT (20));
            Cpu::cpuid (0x7, 0x1, eax, ebx, ecx, edx);
            applied |= BIT (PATCH_CET_SSS) * !(edx & BIT (18));
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
            pbits = eax & BIT_RANGE (7, 0);
            [[fallthrough]];
        case 0x1 ... 0x7:
            Cpu::cpuid (0x80000001, eax, ebx, ecx, edx);
            Hptp::set_mll (edx & BIT (26) ? 2 : 1);
            [[fallthrough]];
        case 0x0:
            break;
    }

    // Key ID bits reduce the usable physical address bits
    Memattr::obits = min (max (pbits, Memattr::pbits_min), Memattr::pbits_max) - Memattr::kbits;
}

void Patch::init()
{
    extern Patch PATCH_S, PATCH_E;

    for (auto p { &PATCH_S }; p < &PATCH_E; p++) {

        if (applied & BIT (p->tag)) [[unlikely]] {

            auto const o { reinterpret_cast<uint8_t *>(p) + p->off_old };
            auto const n { reinterpret_cast<uint8_t *>(p) + p->off_new };

            memcpy (o, n, p->len_new);
            memset (o + p->len_new, NOP_OPC, p->len_old - p->len_new);
        }
    }
}
