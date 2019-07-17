/*
 * Central Processing Unit (CPU)
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

#include "cpu.hpp"
#include "ptab_npt.hpp"
#include "stdio.hpp"

uint64_t Cpu::ptab, Cpu::midr, Cpu::mpidr;

void Cpu::init()
{
    asm volatile ("mrs %0, midr_el1"   : "=r" (midr));
    asm volatile ("mrs %0, mpidr_el1"  : "=r" (mpidr));

    auto impl { "Unknown" }, part { impl };

    switch (midr >> 24 & BIT_RANGE (7, 0)) {

        case 0x41:
            impl = "Arm";
            switch (midr >> 4 & BIT_RANGE (11, 0)) {
                case 0xd02: part = "Cortex-A34 (Metis)";            break;
                case 0xd03: part = "Cortex-A53 (Apollo)";           break;
                case 0xd04: part = "Cortex-A35 (Mercury)";          break;
                case 0xd05: part = "Cortex-A55 (Ananke)";           break;
                case 0xd06: part = "Cortex-A65 (Helios)";           break;
                case 0xd07: part = "Cortex-A57 (Atlas)";            break;
                case 0xd08: part = "Cortex-A72 (Maia)";             break;
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
                case 0xd4d: part = "Cortex-A715 (Makalu)";          break;
                case 0xd4e: part = "Cortex-X3 (Makalu ELP)";        break;
                case 0xd4f: part = "Neoverse V2 (Demeter)";         break;
                case 0xd80: part = "Cortex-A520 (Hayes)";           break;
                case 0xd81: part = "Cortex-A720 (Hunter)";          break;
                case 0xd82: part = "Cortex-X4 (Hunter ELP)";        break;
                case 0xd83: part = "Neoverse (Poseidon VNAE)";      break;
                case 0xd84: part = "Neoverse (Poseidon V)";         break;
                case 0xd85: part = "Cortex (Blackhawk)";            break;
                case 0xd87: part = "Cortex (Chaberton)";            break;
                case 0xd8a: part = "Cortex (Nevis)";                break;
                case 0xd8b: part = "Cortex (Gelas)";                break;
                case 0xd8c: part = "Cortex (Travis)";               break;
                case 0xd8e: part = "Neoverse (Hermes)";             break;
            }
            break;

        case 0x4e:
            impl = "NVIDIA";
            switch (midr >> 4 & BIT_RANGE (11, 0)) {
                case 0x003: part = "Denver";                        break;
                case 0x004: part = "Carmel";                        break;
            }
            break;

        case 0x51:
            impl = "Qualcomm";
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

        case 0x61:
            impl = "Apple";
            switch (midr >> 4 & BIT_RANGE (11, 0)) {
                case 0x001: part = "A7 (Cyclone)";                  break;
                case 0x002: part = "A8 (Typhoon)";                  break;
                case 0x003: part = "A8X (Typhoon)";                 break;
                case 0x004: part = "A9 (Twister)";                  break;
                case 0x005: part = "A9X (Twister)";                 break;
                case 0x006: part = "A10 (Hurricane)";               break;
                case 0x007: part = "A10X (Hurricane)";              break;
                case 0x008: part = "A11 (Monsoon)";                 break;
                case 0x009: part = "A11 (Mistral)";                 break;
                case 0x00b: part = "A12 (Vortex)";                  break;
                case 0x00c: part = "A12 (Tempest)";                 break;
                case 0x010: part = "A12X (Vortex)";                 break;
                case 0x011: part = "A12X (Tempest)";                break;
                case 0x012: part = "A13 (Lightning)";               break;
                case 0x013: part = "A13 (Thunder)";                 break;
                case 0x022: part = "M1 (Icestorm)";                 break;
                case 0x023: part = "M1 (Firestorm)";                break;
                case 0x024: part = "M1 Pro (Icestorm)";             break;
                case 0x025: part = "M1 Pro (Firestorm)";            break;
                case 0x028: part = "M1 Max (Icestorm)";             break;
                case 0x029: part = "M1 Max (Firestorm)";            break;
            }
            break;
    }

    trace (TRACE_CPU, "CORE: %02lu:%02lu:%02lu:%02lu %s %s r%lup%lu",
           mpidr >> 32 & BIT_RANGE (7, 0), mpidr >> 16 & BIT_RANGE (7, 0), mpidr >> 8 & BIT_RANGE (7, 0), mpidr & BIT_RANGE (7, 0),
           impl, part, midr >> 20 & BIT_RANGE (3, 0), midr & BIT_RANGE (3, 0));

    Nptp::init();
}

void Cpu::fini()
{
}
