/*
 * Central Processing Unit (CPU)
 *
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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

#include "cache.hpp"
#include "cpu.hpp"
#include "ptab_npt.hpp"
#include "stdio.hpp"

uint64 Cpu::midr, Cpu::mpidr;

void Cpu::init()
{
    asm volatile ("mrs %0, midr_el1"   : "=r" (midr));
    asm volatile ("mrs %0, mpidr_el1"  : "=r" (mpidr));

    char const *impl = "Unknown", *part = impl;

    switch (midr >> 24 & BIT_RANGE (7, 0)) {

        case 0x41:
            impl = "ARM Limited";
            switch (midr >> 4 & BIT_RANGE (11, 0)) {
                case 0xd02: part = "Cortex-A34 (Metis)";            break;
                case 0xd03: part = "Cortex-A53 (Apollo)";           break;
                case 0xd04: part = "Cortex-A35 (Mercury)";          break;
                case 0xd05: part = "Cortex-A55 (Ananke)";           break;
                case 0xd06: part = "Cortex-A65 (Helios)";           break;
                case 0xd07: part = "Cortex-A57 (Atlas)";            break;
                case 0xd08: part = "Cortex-A72 (Maya)";             break;
                case 0xd09: part = "Cortex-A73 (Artemis)";          break;
                case 0xd0a: part = "Cortex-A75 (Prometheus)";       break;
                case 0xd0b: part = "Cortex-A76 (Enyo)";             break;
                case 0xd0c: part = "Neoverse N1 (Ares)";            break;
                case 0xd0d: part = "Cortex-A77 (Deimos)";           break;
                case 0xd0e: part = "Cortex-A76AE (Enyo-AE)";        break;
                case 0xd40: part = "Neoverse V1 (Zeus)";            break;
                case 0xd41: part = "Cortex-A78 (Hercules)";         break;
                case 0xd42: part = "Cortex-A78AE (Hercules-AE)";    break;
                case 0xd43: part = "Cortex-A65AE (Helios-AE)";      break;
                case 0xd44: part = "Cortex-X1 (Hera)";              break;
                case 0xd46: part = "Cortex-A510 (Klein)";           break;
                case 0xd47: part = "Cortex-A710 (Matterhorn)";      break;
                case 0xd48: part = "Cortex-X2 (Matterhorn ELP)";    break;
                case 0xd49: part = "Neoverse N2 (Perseus)";         break;
                case 0xd4a: part = "Neoverse E1 (Helios)";          break;
                case 0xd4b: part = "Cortex-A78C (Hercules-C)";      break;
                case 0xd4c: part = "Cortex-X1C (Hera-C)";           break;
                case 0xd4d: part = "Cortex (Makalu)";               break;
                case 0xd4e: part = "Cortex (Makalu ELP)";           break;
                case 0xd4f: part = "Neoverse (Demeter)";            break;
                case 0xd80: part = "Cortex (Hayes)";                break;
                case 0xd81: part = "Cortex (Hunter)";               break;
                case 0xd83: part = "Neoverse (Poseidon)";           break;
            }
            break;

        case 0x4e:
            impl = "NVIDIA Corporation";
            switch (midr >> 4 & BIT_RANGE (11, 0)) {
                case 0x003: part = "Denver";                        break;
                case 0x004: part = "Carmel";                        break;
            }
            break;

        case 0x51:
            impl = "Qualcomm Inc.";
            switch (midr >> 4 & BIT_RANGE (11, 0)) {
                case 0x800: part = "Kryo 2xx Gold";                 break;
                case 0x801: part = "Kryo 2xx Silver";               break;
                case 0x802: part = "Kryo 3xx Gold";                 break;
                case 0x803: part = "Kryo 3xx Silver";               break;
                case 0x804: part = "Kryo 4xx Gold";                 break;
                case 0x805: part = "Kryo 4xx Silver";               break;
                case 0xc00: part = "Falkor";                        break;
                case 0xc01: part = "Saphira";                       break;
            }
            break;
    }

    trace (TRACE_CPU, "CORE: %x:%x:%x %s %s r%llup%llu",
           affinity() >> 16 & BIT_RANGE (7, 0), affinity() >> 8 & BIT_RANGE (7, 0), affinity() & BIT_RANGE (7, 0),
           impl, part, midr >> 20 & BIT_RANGE (3, 0), midr & BIT_RANGE (3, 0));

    Nptp::init();
}

void Cpu::fini()
{
}
