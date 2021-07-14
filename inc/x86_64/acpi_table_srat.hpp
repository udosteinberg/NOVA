/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

#pragma once

#include "acpi_table.hpp"

/*
 * 5.2.16: System Resource Affinity Table (SRAT)
 */
class Acpi_table_srat final
{
    private:
        Acpi_table  table;                              //  0
        uint32_t    reserved1;                          // 36
        uint64_t    reserved2;                          // 40

        struct Affinity
        {
            enum Type : uint8_t
            {
                LAPIC   = 0,                            // Local APIC
                MEMORY  = 1,                            // Memory
                X2APIC  = 2,                            // Local x2APIC
            };

            Type        type;                           //  0 + n
            uint8_t     length;                         //  1 + n
        };

        /*
         * 5.2.16.1: Local APIC Affinity Structure
         */
        struct Affinity_lapic final : Affinity
        {
            enum Flags : uint32_t
            {
                ENABLED     = BIT (0),
            };

            uint8_t     pxd0;                           //  2 + n
            uint8_t     id;                             //  3 + n
            Flags       flags;                          //  4 + n
            uint8_t     eid;                            //  8 + n
            uint8_t     pxd1, pxd2, pxd3;               //  9 + n
            uint32_t    clock;                          // 12 + n

            void parse() const;
        };

        static_assert (sizeof (Affinity_lapic) == 16);

        /*
         * 5.2.16.2: Memory Affinity Structure
         */
#pragma pack(1)
        struct Affinity_memory final : Affinity
        {
            enum Flags : uint32_t
            {
                ENABLED     = BIT (0),
                HOTPLUG     = BIT (1),
                NONVOLATILE = BIT (2),
            };

            uint32_t    pxd;                            //  2 + n
            uint16_t    reserved1;                      //  6 + n
            uint64_t    base;                           //  8 + n
            uint64_t    size;                           // 16 + n
            uint32_t    reserved2;                      // 24 + n
            Flags       flags;                          // 28 + n
            uint64_t    reserved3;                      // 32 + n

            void parse() const;
        };
#pragma pack()

        static_assert (sizeof (Affinity_memory) == 40);

        /*
         * 5.2.16.3: x2APIC Affinity Structure
         */
        struct Affinity_x2apic : Affinity
        {
            using Flags = Affinity_lapic::Flags;

            uint16_t    reserved1;                      //  2 + n
            uint32_t    pxd;                            //  4 + n
            uint32_t    id;                             //  8 + n
            Flags       flags;                          // 12 + n
            uint32_t    clock;                          // 16 + n
            uint32_t    reserved2;                      // 20 + n

            void parse() const;
        };

        static_assert (sizeof (Affinity_x2apic) == 24);

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_srat) && sizeof (Acpi_table_srat) == 48);
