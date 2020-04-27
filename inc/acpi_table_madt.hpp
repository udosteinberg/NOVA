/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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
 * 5.2.12: Multiple APIC Description Table (MADT)
 */
class Acpi_table_madt final
{
    private:
        enum Flags : uint32_t
        {
            PCAT_COMPAT         = BIT (0),
        };

        Acpi_table  table;                              //  0
        uint32_t    phys;                               // 36
        Flags       flags;                              // 40

        /*
         * Each Controller struct is only 32bit (but not 64bit) aligned. This is because
         * the first struct is at offset n=44 and each struct has a multiple-of-4 size.
         */
        struct Controller
        {
            enum Type : uint8_t
            {
                LAPIC   = 0,                            // Local APIC
                IOAPIC  = 1,                            // I/O APIC
                X2APIC  = 9,                            // Local x2APIC
                GICC    = 11,                           // GIC CPU Interface
                GICD    = 12,                           // GIC Distributor
                GMSI    = 13,                           // GIC MSI Frame
                GICR    = 14,                           // GIC Redistributor
                GITS    = 15,                           // GIC Interrupt Translation Service
            };

            Type        type;                           //  0 + n
            uint8_t     length;                         //  1 + n
        };

        /*
         * 5.2.12.2: Local APIC Structure
         */
        struct Controller_lapic : Controller
        {
            enum Flags : uint32_t
            {
                ENABLED             = BIT (0),
                ONLINE_CAPABLE      = BIT (1),
            };

            uint8_t     acpi_uid;                       //  2 + n
            uint8_t     id;                             //  3 + n
            Flags       flags;                          //  4 + n
                                                        //  8 + n
            void parse() const;
        };

        /*
         * 5.2.12.3: I/O APIC Structure
         */
        struct Controller_ioapic : Controller
        {
            uint8_t     id;                             //  2 + n
            uint8_t     reserved;                       //  3 + n
            uint32_t    phys;                           //  4 + n
            uint32_t    gsi;                            //  8 + n
                                                        // 12 + n
            void parse() const;
        };

        /*
         * 5.2.12.12: x2APIC Structure
         */
        struct Controller_x2apic : Controller
        {
            using Flags = Controller_lapic::Flags;

            uint16_t    reserved;                       //  2 + n
            uint32_t    id;                             //  4 + n
            Flags       flags;                          //  8 + n
            uint32_t    acpi_uid;                       // 12 + n
                                                        // 16 + n
            void parse() const;
        };

        /*
         * 5.2.12.14: GIC CPU Interface (GICC) Structure
         */
        struct Controller_gicc : Controller
        {
            enum Flags : uint32_t
            {
                ENABLED             = BIT (0),
                TRIGGER_EDGE_PERF   = BIT (1),
                TRIGGER_EDGE_VGIC   = BIT (2),
            };

            uint16_t    reserved1;                      //  2 + n (5.0)
            uint32_t    cpu;                            //  4 + n (5.0)
            uint32_t    uid;                            //  8 + n (5.0)
            Flags       flags;                          // 12 + n (5.0)
            uint32_t    park_pver;                      // 16 + n (5.0)
            uint32_t    gsiv_perf;                      // 20 + n (5.0)
            uint32_t    phys_park_lo, phys_park_hi;     // 24 + n (5.0)
            uint32_t    phys_gicc_lo, phys_gicc_hi;     // 32 + n (5.0)
            uint32_t    phys_gicv_lo, phys_gicv_hi;     // 40 + n (5.1)
            uint32_t    phys_gich_lo, phys_gich_hi;     // 48 + n (5.1)
            uint32_t    gsiv_vgic;                      // 56 + n (5.1)
            uint32_t    phys_gicr_lo, phys_gicr_hi;     // 60 + n (5.1)
            uint32_t    val_mpidr_lo, val_mpidr_hi;     // 68 + n (5.1)
            uint8_t     ppec;                           // 76 + n (6.0)
            uint8_t     reserved2[3];                   // 77 + n (6.0)
                                                        // 80 + n
            void parse() const;
        };

        /*
         * 5.2.12.15: GIC Distributor (GICD) Structure
         */
        struct Controller_gicd : Controller
        {
            uint16_t    reserved1;                      //  2 + n (5.0)
            uint32_t    hid;                            //  4 + n (5.0)
            uint32_t    phys_gicd_lo, phys_gicd_hi;     //  8 + n (5.0)
            uint32_t    vect_base;                      // 16 + n (5.0)
            uint8_t     version;                        // 20 + n (5.0)
            uint8_t     reserved2[3];                   // 21 + n (5.0)
                                                        // 24 + n
            void parse() const;
        };

        /*
         * 5.2.12.16: GIC MSI Frame (GMSI) Structure
         */
        struct Controller_gmsi : Controller
        {
            uint16_t    reserved1;                      //  2 + n (5.1)
            uint32_t    id;                             //  4 + n (5.1)
            uint32_t    phys_gmsi_lo, phys_gmsi_hi;     //  8 + n (5.1)
            uint32_t    flags;                          // 16 + n (5.1)
            uint16_t    spi_count;                      // 20 + n (5.1)
            uint16_t    spi_base;                       // 22 + n (5.1)
                                                        // 24 + n
            void parse() const;
        };

        /*
         * 5.2.12.17: GIC Redistributor (GICR) Structure
         */
        struct Controller_gicr : Controller
        {
            uint16_t    reserved1;                      //  2 + n (5.1)
            uint32_t    phys_gicr_lo, phys_gicr_hi;     //  4 + n (5.1)
            uint32_t    size_gicr;                      // 12 + n (5.1)
                                                        // 16 + n
            void parse() const;
        };

        /*
         * 5.2.12.18: GIC Interrupt Translation Service (GITS) Structure
         */
        struct Controller_gits : Controller
        {
            uint16_t    reserved1;                      //  2 + n (6.0)
            uint32_t    id;                             //  4 + n (6.0)
            uint32_t    phys_gits_lo, phys_gits_hi;     //  8 + n (6.0)
            uint32_t    reserved2;                      // 16 + n (6.0)
                                                        // 20 + n
            void parse() const;
        };

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_madt) && sizeof (Acpi_table_madt) == 44);
