/*
 * Advanced Configuration and Power Interface (ACPI)
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

#pragma once

#include "acpi_table.hpp"

/*
 * 5.2.12: Multiple APIC Description Table (MADT)
 */
class Acpi_table_madt : private Acpi_table
{
    /*
     * Each Controller struct is only 32bit (but not 64bit) aligned. This is because
     * the first struct is at offset n=44 and each struct has a multiple-of-4 size.
     */
    struct Controller
    {
        enum Type : uint8
        {
            GICC    = 11,                           // GIC CPU Interface
            GICD    = 12,                           // GIC Distributor
            GMSI    = 13,                           // GIC MSI Frame
            GICR    = 14,                           // GIC Redistributor
            GITS    = 15,                           // GIC Interrupt Translation Service
        };

        Type        type;                           //  0 + n
        uint8       length;                         //  1 + n
    };

    /*
     * 5.2.12.14: GIC CPU Interface (GICC) Structure
     */
    struct Controller_gicc : Controller
    {
        enum Flags : uint32
        {
            ENABLED             = BIT (0),
            TRIGGER_EDGE_PERF   = BIT (1),
            TRIGGER_EDGE_VGIC   = BIT (2),
        };

        uint16      reserved1;                      //  2 + n (5.0)
        uint32      cpu;                            //  4 + n (5.0)
        uint32      uid;                            //  8 + n (5.0)
        Flags       flags;                          // 12 + n (5.0)
        uint32      park_pver;                      // 16 + n (5.0)
        uint32      gsiv_perf;                      // 20 + n (5.0)
        uint32      phys_park_lo, phys_park_hi;     // 24 + n (5.0)
        uint32      phys_gicc_lo, phys_gicc_hi;     // 32 + n (5.0)
        uint32      phys_gicv_lo, phys_gicv_hi;     // 40 + n (5.1)
        uint32      phys_gich_lo, phys_gich_hi;     // 48 + n (5.1)
        uint32      gsiv_vgic;                      // 56 + n (5.1)
        uint32      phys_gicr_lo, phys_gicr_hi;     // 60 + n (5.1)
        uint32      val_mpidr_lo, val_mpidr_hi;     // 68 + n (5.1)
        uint8       ppec;                           // 76 + n (6.0)
        uint8       reserved2[3];                   // 77 + n (6.0)
                                                    // 80 + n
        void parse() const;
    };

    /*
     * 5.2.12.15: GIC Distributor (GICD) Structure
     */
    struct Controller_gicd : Controller
    {
        uint16      reserved1;                      //  2 + n (5.0)
        uint32      hid;                            //  4 + n (5.0)
        uint32      phys_gicd_lo, phys_gicd_hi;     //  8 + n (5.0)
        uint32      vect_base;                      // 16 + n (5.0)
        uint8       version;                        // 20 + n (5.0)
        uint8       reserved2[3];                   // 21 + n (5.0)
                                                    // 24 + n
        void parse() const;
    };

    /*
     * 5.2.12.16: GIC MSI Frame Structure
     */
    struct Controller_gmsi : Controller
    {
        uint16      reserved1;                      //  2 + n (5.1)
        uint32      id;                             //  4 + n (5.1)
        uint32      phys_gmsi_lo, phys_gmsi_hi;     //  8 + n (5.1)
        uint32      flags;                          // 16 + n (5.1)
        uint16      spi_count;                      // 20 + n (5.1)
        uint16      spi_base;                       // 22 + n (5.1)
                                                    // 24 + n
        void parse() const;
    };

    /*
     * 5.2.12.17: GIC Redistributor (GICR) Structure
     */
    struct Controller_gicr : Controller
    {
        uint16      reserved1;                      //  2 + n (5.1)
        uint32      phys_gicr_lo, phys_gicr_hi;     //  4 + n (5.1)
        uint32      size_gicr;                      // 12 + n (5.1)
                                                    // 16 + n
        void parse() const;
    };

    /*
     * 5.2.12.18: GIC Interrupt Translation Service (GITS) Structure
     */
    struct Controller_gits : Controller
    {
        uint16      reserved1;                      //  2 + n (6.0)
        uint32      id;                             //  4 + n (6.0)
        uint32      phys_gits_lo, phys_gits_hi;     //  8 + n (6.0)
        uint32      reserved2;                      // 16 + n (6.0)
                                                    // 20 + n
        void parse() const;
    };

    private:
        uint32      phys;                           // 36
        uint32      flags;                          // 40
        uint8       data;                           // 44

    public:
        void parse() const;
};
