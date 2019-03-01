/*
 * Central Processing Unit (CPU)
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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
#include "stdio.hpp"

void Cpu::init()
{
    uint64 midr, mpidr;

    asm volatile ("mrs %0, midr_el1"  : "=r" (midr));
    asm volatile ("mrs %0, mpidr_el1" : "=r" (mpidr));

    char const *impl = "Unknown", *part = impl;

    switch (midr >> 24 & 0xff) {

        case 0x41:
            impl = "ARM Limited";
            switch (midr >> 4 & 0xfff) {
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
                case 0xd46: part = "Cortex-A (Klein)";              break;
                case 0xd47: part = "Cortex-A (Matterhorn)";         break;
                case 0xd4a: part = "Neoverse E1 (Helios)";          break;
            }
            break;

        case 0x51:
            impl = "Qualcomm Inc.";
            switch (midr >> 4 & 0xfff) {
                case 0x802: part = "Kryo 3xx Gold";                 break;
                case 0x803: part = "Kryo 3xx Silver";               break;
                case 0x804: part = "Kryo 4xx Gold";                 break;
                case 0x805: part = "Kryo 4xx Silver";               break;
            }
            break;
    }

    uint8  aff0  = mpidr       & 0xff;
    uint8  aff1  = mpidr >>  8 & 0xff;
    uint8  aff2  = mpidr >> 16 & 0xff;

    trace (TRACE_CPU, "CORE: %x:%x:%x %s %20s r%llup%llu", aff2, aff1, aff0, impl, part, midr >> 20 & 0xf, midr & 0xf);
}
