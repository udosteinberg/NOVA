/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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
 * 18.3.2: Hardware Error Source Table (HEST)
 */
class Acpi_table_hest final
{
    private:
        Acpi_table                  table;                      // 0
        Unaligned_le<uint32_t>      count;                      // 36

        struct Source : private Unaligned_le<uint16_t>          // 0
        {
            Unaligned_le<uint16_t>  src_id;                     // 2

            enum class Type : uint16_t
            {
                MCE     = 0,                                    // IA32 Machine Check Exception
                CMC     = 1,                                    // IA32 Corrected Machine Check
                NMI     = 2,                                    // IA32 Non-Maskable Interrupt
                AER_RP  = 6,                                    // PCIE AER Root Port
                AER_EP  = 7,                                    // PCIE AER Endpoint
                AER_BR  = 8,                                    // PCIE AER Bridge
                GHES_1  = 9,                                    // Generic Hardware Error Source (v1)
                GHES_2  = 10,                                   // Generic Hardware Error Source (v2)
                DMC     = 11,                                   // IA32 Deferred Machine Check
            };

            auto type() const { return Type { uint16_t { *this } }; }
        };

        static_assert (alignof (Source) == 1 && sizeof (Source) == 4);

        /*
         * 18.3.2.9 Hardware Error Notiﬁcation
         */
        struct Notification : private Unaligned_le<uint8_t>     // 0
        {
            Unaligned_le<uint8_t>   length;                     // 1
            Unaligned_le<uint16_t>  cfg;                        // 2
            Unaligned_le<uint32_t>  poll;                       // 4
            Unaligned_le<uint32_t>  vector;                     // 8
            Unaligned_le<uint32_t>  t_val;                      // 12
            Unaligned_le<uint32_t>  t_win;                      // 16
            Unaligned_le<uint32_t>  e_val;                      // 20
            Unaligned_le<uint32_t>  e_win;                      // 24

            enum class Type : uint8_t
            {
                POLLED  = 0,                                    // Polled
                GSI     = 1,                                    // External Interrupt (GSI)
                LINT    = 2,                                    // Local Interrupt
                SCI     = 3,                                    // SCI
                NMI     = 4,                                    // NMI
                CMCI    = 5,                                    // CMCI
                MCE     = 6,                                    // MCE
                GPIO    = 7,                                    // GPIO
                SEA     = 8,                                    // Armv8 SEA
                SEI     = 9,                                    // Armv8 SEI
                GSIV    = 10,                                   // External Interrupt (GSIV)
                SDEI    = 11,                                   // Software Delegated Exception
            };

            auto type() const { return Type { uint8_t { *this } }; }
        };

        static_assert (alignof (Notification) == 1 && sizeof (Notification) == 28);

        /*
         * 18.3.2.7 Generic Hardware Error Source (Type 9)
         */
        struct Source_ghes_1 : public Source                    // 0
        {
            Unaligned_le<uint16_t>  rel_id;                     // 4
            Unaligned_le<uint8_t>   flags;                      // 6
            Unaligned_le<uint8_t>   enabled;                    // 7
            Unaligned_le<uint32_t>  rec;                        // 8
            Unaligned_le<uint32_t>  sec;                        // 12
            Unaligned_le<uint32_t>  raw;                        // 16
            Acpi_gas                status;                     // 20
            Notification            notify;                     // 32
            Unaligned_le<uint32_t>  length;                     // 60

            size_t parse() const;
        };

        static_assert (alignof (Source_ghes_1) == 1 && sizeof (Source_ghes_1) == 64);

        /*
         * 18.3.2.6 PCI Express/PCI-X Bridge AER Structure (Type 8)
         */
        struct Source_aer_br : public Source                    // 0
        {
            Unaligned_le<uint8_t>   fixme[52];                  // 4

            size_t parse() const { return sizeof (*this); }
        };

        static_assert (alignof (Source_aer_br) == 1 && sizeof (Source_aer_br) == 56);

        /*
         * 18.3.2.5 PCI Express Device AER Structure (Type 7)
         */
        struct Source_aer_ep : public Source                    // 0
        {
            Unaligned_le<uint8_t>   fixme[40];                  // 4

            size_t parse() const { return sizeof (*this); }
        };

        static_assert (alignof (Source_aer_ep) == 1 && sizeof (Source_aer_ep) == 44);

        /*
         * 18.3.2.4 PCI Express Root Port AER Structure (Type 6)
         */
        struct Source_aer_rp : public Source                    // 0
        {
            Unaligned_le<uint8_t>   fixme[44];                  // 4

            size_t parse() const { return sizeof (*this); }
        };

        static_assert (alignof (Source_aer_rp) == 1 && sizeof (Source_aer_rp) == 48);

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_hest) && alignof (Acpi_table_hest) == 1 && sizeof (Acpi_table_hest) == 40);
