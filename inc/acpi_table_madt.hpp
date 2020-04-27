/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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
 * 5.2.12: Multiple APIC Description Table (MADT)
 */
class Acpi_table_madt final
{
    private:
        Acpi_table                  table;                      // 0
        Unaligned_le<uint32_t>      phys;                       // 36
        Unaligned_le<uint32_t>      flags;                      // 40

        struct Controller : protected Unaligned_le<uint8_t>     // 0
        {
            Unaligned_le<uint8_t>   length;                     // 1

            enum class Type : uint8_t
            {
                LAPIC   = 0,                                    // Local APIC
                IOAPIC  = 1,                                    // I/O APIC
                X2APIC  = 9,                                    // Local x2APIC
                GICC    = 11,                                   // GIC CPU Interface
                GICD    = 12,                                   // GIC Distributor
                GMSI    = 13,                                   // GIC MSI Frame
                GICR    = 14,                                   // GIC Redistributor
                GITS    = 15,                                   // GIC Interrupt Translation Service
            };

            auto type() const { return Type { uint8_t { *this } }; }
        };

        static_assert (alignof (Controller) == 1 && sizeof (Controller) == 2);

        /*
         * 5.2.12.2: Local APIC Structure
         */
        struct Controller_lapic : public Controller             // 0
        {
            Unaligned_le<uint8_t>   uid;                        // 2
            Unaligned_le<uint8_t>   id;                         // 3
            Unaligned_le<uint32_t>  flags;                      // 4

            void parse() const;
        };

        static_assert (alignof (Controller_lapic) == 1 && sizeof (Controller_lapic) == 8);

        /*
         * 5.2.12.3: I/O APIC Structure
         */
        struct Controller_ioapic : public Controller            // 0
        {
            Unaligned_le<uint8_t>   id;                         // 2
            Unaligned_le<uint8_t>   reserved;                   // 3
            Unaligned_le<uint32_t>  phys;                       // 4
            Unaligned_le<uint32_t>  gsi;                        // 8

            void parse() const;
        };

        static_assert (alignof (Controller_ioapic) == 1 && sizeof (Controller_ioapic) == 12);

        /*
         * 5.2.12.12: x2APIC Structure
         */
        struct Controller_x2apic : public Controller            // 0
        {
            Unaligned_le<uint16_t>  reserved;                   // 2
            Unaligned_le<uint32_t>  id;                         // 4
            Unaligned_le<uint32_t>  flags;                      // 8
            Unaligned_le<uint32_t>  uid;                        // 12

            void parse() const;
        };

        static_assert (alignof (Controller_x2apic) == 1 && sizeof (Controller_x2apic) == 16);

        /*
         * 5.2.12.14: GIC CPU Interface (GICC) Structure
         */
        struct Controller_gicc : public Controller              // 0
        {
            Unaligned_le<uint16_t>  reserved1;                  // 2    5.0+
            Unaligned_le<uint32_t>  cpu;                        // 4    5.0+
            Unaligned_le<uint32_t>  uid;                        // 8    5.0+
            Unaligned_le<uint32_t>  flags;                      // 12   5.0+
            Unaligned_le<uint32_t>  park_pver;                  // 16   5.0+
            Unaligned_le<uint32_t>  gsiv_perf;                  // 20   5.0+
            Unaligned_le<uint64_t>  phys_park;                  // 24   5.0+
            Unaligned_le<uint64_t>  phys_gicc;                  // 32   5.0+
            Unaligned_le<uint64_t>  phys_gicv;                  // 40   5.1+
            Unaligned_le<uint64_t>  phys_gich;                  // 48   5.1+
            Unaligned_le<uint32_t>  gsiv_vgic;                  // 56   5.1+
            Unaligned_le<uint64_t>  phys_gicr;                  // 60   5.1+
            Unaligned_le<uint64_t>  val_mpidr;                  // 68   5.1+
            Unaligned_le<uint8_t>   ppec;                       // 76   6.0+
            Unaligned_le<uint8_t>   reserved2[3];               // 77   6.0+

            void parse() const;
        };

        static_assert (alignof (Controller_gicc) == 1 && sizeof (Controller_gicc) == 80);

        /*
         * 5.2.12.15: GIC Distributor (GICD) Structure
         */
        struct Controller_gicd : public Controller              // 0
        {
            Unaligned_le<uint16_t>  reserved1;                  // 2    5.0+
            Unaligned_le<uint32_t>  hid;                        // 4    5.0+
            Unaligned_le<uint64_t>  phys_gicd;                  // 8    5.0+
            Unaligned_le<uint32_t>  vect_base;                  // 16   5.0+
            Unaligned_le<uint8_t>   version;                    // 20   5.0+
            Unaligned_le<uint8_t>   reserved2[3];               // 21   5.0+

            void parse() const;
        };

        static_assert (alignof (Controller_gicd) == 1 && sizeof (Controller_gicd) == 24);

        /*
         * 5.2.12.16: GIC MSI Frame (GMSI) Structure
         */
        struct Controller_gmsi : public Controller              // 0
        {
            Unaligned_le<uint16_t>  reserved1;                  // 2    5.1+
            Unaligned_le<uint32_t>  id;                         // 4    5.1+
            Unaligned_le<uint64_t>  phys_gmsi;                  // 8    5.1+
            Unaligned_le<uint32_t>  flags;                      // 16   5.1+
            Unaligned_le<uint16_t>  spi_count;                  // 20   5.1+
            Unaligned_le<uint16_t>  spi_base;                   // 22   5.1+

            void parse() const;
        };

        static_assert (alignof (Controller_gmsi) == 1 && sizeof (Controller_gmsi) == 24);

        /*
         * 5.2.12.17: GIC Redistributor (GICR) Structure
         */
        struct Controller_gicr : public Controller              // 0
        {
            Unaligned_le<uint16_t>  reserved1;                  // 2    5.1+
            Unaligned_le<uint64_t>  phys_gicr;                  // 4    5.1+
            Unaligned_le<uint32_t>  size_gicr;                  // 12   5.1+

            void parse() const;
        };

        static_assert (alignof (Controller_gicr) == 1 && sizeof (Controller_gicr) == 16);

        /*
         * 5.2.12.18: GIC Interrupt Translation Service (GITS) Structure
         */
        struct Controller_gits : public Controller              // 0
        {
            Unaligned_le<uint16_t>  reserved1;                  // 2    6.0+
            Unaligned_le<uint32_t>  id;                         // 4    6.0+
            Unaligned_le<uint64_t>  phys_gits;                  // 8    6.0+
            Unaligned_le<uint32_t>  reserved2;                  // 16   6.0+

            void parse() const;
        };

        static_assert (alignof (Controller_gits) == 1 && sizeof (Controller_gits) == 20);

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_madt) && alignof (Acpi_table_madt) == 1 && sizeof (Acpi_table_madt) == 44);
