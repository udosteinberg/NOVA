/*
 * Advanced Configuration and Power Interface (ACPI)
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

#pragma once

#include "acpi_table.hpp"

/*
 * 5.2: I/O Virtualization Reporting Structure (IVRS)
 */
class Acpi_table_ivrs final
{
    private:
        Acpi_table                  table;                      // 0
        Unaligned_le<uint32_t>      ivinfo;                     // 36
        Unaligned_le<uint64_t>      reserved;                   // 40

        /*
         * 5.2.2: I/O Virtualization Definition Block
         */
        struct Block : private Unaligned_le<uint8_t>            // 0
        {
            Unaligned_le<uint8_t>   flags;                      // 1
            Unaligned_le<uint16_t>  len;                        // 2

            enum class Type : uint8_t
            {
                IVHD_10             = 0x10,                     // Deprecated
                IVHD_11             = 0x11,
                IVHD_40             = 0x40,

                IVMD_ALL            = 0x20,                     // Peripherals (All)
                IVMD_SPEC           = 0x21,                     // Peripherals (Specified)
                IVMD_RANGE          = 0x22,                     // Peripherals (Range)
            };

            auto type() const { return Type { uint8_t { *this } }; }
        };

        static_assert (alignof (Block) == 1 && sizeof (Block) == 4);

        /*
         * 5.2.2.1: I/O Virtualization Hardware Definition (Type 11h IVHD) Block
         */
        struct Ivhd_11 final : public Block                     // 0
        {
            Unaligned_le<uint16_t>  bdf;                        // 4
            Unaligned_le<uint16_t>  cap;                        // 6
            Unaligned_le<uint64_t>  phys;                       // 8
            Unaligned_le<uint16_t>  seg;                        // 16
            Unaligned_le<uint16_t>  info;                       // 18
            Unaligned_le<uint32_t>  attr;                       // 20
            Unaligned_le<uint64_t>  efr1;                       // 24
            Unaligned_le<uint64_t>  efr2;                       // 32

            [[nodiscard]] bool parse() const;
        };

        static_assert (alignof (Ivhd_11) == 1 && sizeof (Ivhd_11) == 40);

        struct Device : private Unaligned_le<uint8_t>           // 0
        {
            enum class Type : uint8_t
            {
                ALL                 = 0x01,                     // All Devices
                DEV_SELECT          = 0x02,                     // Device
                DEV_RANGE_S         = 0x03,                     // Device Range Start
                DEV_RANGE_E         = 0x04,                     // Device Range End
                ALS_SELECT          = 0x42,                     // Alias
                ALS_RANGE_S         = 0x43,                     // Alias Range Start
                EXT_SELECT          = 0x46,                     // Extended Device
                EXT_RANGE           = 0x47,                     // Extended Range Start
                SPECIAL             = 0x48,                     // Special Device
            };

            auto type() const { return Type { uint8_t { *this } }; }

            // Table 101: IVHD Device Entry Length Based on Type
            unsigned len() const { return uint8_t { *this } < 64 ? 4 : uint8_t { *this } < 128 ? 8 : 0; }
        };

        static_assert (alignof (Device) == 1 && sizeof (Device) == 1);

        /*
         * Device Entry: 4-bytes
         */
        struct Device_4 : public Device                         // 0
        {
            Unaligned_le<uint16_t>  bdf;                        // 1
            Unaligned_le<uint8_t>   dte;                        // 3
        };

        static_assert (alignof (Device_4) == 1 && sizeof (Device_4) == 4);

        /*
         * Device Entry: 8-bytes
         */
        struct Device_8 : public Device_4                       // 0
        {
            Unaligned_le<uint8_t>   handle;                     // 4
            Unaligned_le<uint16_t>  src;                        // 5
            Unaligned_le<uint8_t>   variety;                    // 7
        };

        static_assert (alignof (Device_8) == 1 && sizeof (Device_8) == 8);

    public:
        [[nodiscard]] bool parse() const;
};

static_assert (__is_standard_layout (Acpi_table_ivrs) && alignof (Acpi_table_ivrs) == 1 && sizeof (Acpi_table_ivrs) == 48);
