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
         * 5.2.2: I/O Virtualization Definition Block (IVDB)
         */
        struct Ivdb : private Unaligned_le<uint8_t>             // 0
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

        static_assert (alignof (Ivdb) == 1 && sizeof (Ivdb) == 4);

        /*
         * Table 89: I/O Virtualization Hardware Definition (IVHD) - Generic Format
         */
        class Ivhd : public Ivdb                                // 0
        {
            protected:
                Unaligned_le<uint16_t>  bdf;                    // 4
                Unaligned_le<uint16_t>  cap;                    // 6
                Unaligned_le<uint64_t>  phys;                   // 8
                Unaligned_le<uint16_t>  seg;                    // 16
                Unaligned_le<uint16_t>  info;                   // 18
                Unaligned_le<uint32_t>  attr;                   // 20

                [[nodiscard]] bool parse (size_t, uint64_t, uint64_t) const;
        };

        static_assert (alignof (Ivhd) == 1 && sizeof (Ivhd) == 24);

        /*
         * Table 90: I/O Virtualization Hardware Definition (IVHD) - Type 10h
         */
        struct Ivhd_10 : public Ivhd                            // 0
        {
            [[nodiscard]] bool parse() const { return len < sizeof (*this) ? false : Ivhd::parse (sizeof (*this), 0, 0); }
        };

        static_assert (alignof (Ivhd_10) == 1 && sizeof (Ivhd_10) == 24);

        /*
         * Table 95: I/O Virtualization Hardware Definition (IVHD) - Type 11h
         */
        struct Ivhd_11 : public Ivhd_10                         // 0
        {
            Unaligned_le<uint64_t>  efr1;                       // 24
            Unaligned_le<uint64_t>  efr2;                       // 32

            [[nodiscard]] bool parse() const { return len < sizeof (*this) ? false : Ivhd::parse (sizeof (*this), efr1, efr2); }
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
                ALI_SELECT          = 0x42,                     // Alias
                ALI_RANGE_S         = 0x43,                     // Alias Range Start
                EXT_SELECT          = 0x46,                     // Extended Device
                EXT_RANGE           = 0x47,                     // Extended Range Start
                SPECIAL             = 0x48,                     // Special Device
            };

            auto type() const { return Type { uint8_t { *this } }; }

            // Table 103: IVHD Device Entry Length Based on Type
            size_t len (uintptr_t end) const
            {
                uint8_t const t { *this };

                if (t < 64)
                    return sizeof (Device_4);

                if (t < 128)
                    return sizeof (Device_8);

                if (t == 0xf0)
                    return reinterpret_cast<uintptr_t>(this) + sizeof (Device_f0) > end ? 0 : sizeof (Device_f0) + std::start_lifetime_as<Device_f0 const> (this)->len;

                return 0;
            }
        };

        static_assert (alignof (Device) == 1 && sizeof (Device) == 1);

        /*
         * Table 105: Device Entry Type 0x00 ... 0x3f
         */
        struct Device_4 : public Device                         // 0
        {
            Unaligned_le<uint16_t>  bdf;                        // 1
            Unaligned_le<uint8_t>   dte;                        // 3
        };

        static_assert (alignof (Device_4) == 1 && sizeof (Device_4) == 4);

        /*
         * Table 107: Device Entry Type 0x40 ... 0x7f
         */
        struct Device_8 : public Device_4                       // 0
        {
            Unaligned_le<uint8_t>   handle;                     // 4
            Unaligned_le<uint16_t>  src;                        // 5
            Unaligned_le<uint8_t>   variety;                    // 7
        };

        static_assert (alignof (Device_8) == 1 && sizeof (Device_8) == 8);

        /*
         * Table 110: Device Entry Type 0xf0
         */
        struct Device_f0 : public Device_4                      // 0
        {
            Unaligned_le<uint64_t>  hid;                        // 4
            Unaligned_le<uint64_t>  cid;                        // 12
            Unaligned_le<uint8_t>   fmt;                        // 20
            Unaligned_le<uint8_t>   len;                        // 21
        };

        static_assert (alignof (Device_f0) == 1 && sizeof (Device_f0) == 22);

        static void parse_entry (Ivdb::Type, uintptr_t, uintptr_t, bool &);

    public:
        [[nodiscard]] bool parse() const;
};

static_assert (__is_standard_layout (Acpi_table_ivrs) && alignof (Acpi_table_ivrs) == 1 && sizeof (Acpi_table_ivrs) == 48);
