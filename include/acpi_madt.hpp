/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#pragma pack(1)

/*
 * ACPI INTI Flags (5.2.11.8)
 */
class Acpi_inti
{
    public:
        uint16  pol :  2,
                trg :  2,
                    : 12;

        enum Polarity
        {
            POL_CONFORMING  = 0,
            POL_HIGH        = 1,
            POL_LOW         = 3,
        };

        enum Trigger
        {
            TRG_CONFORMING  = 0,
            TRG_EDGE        = 1,
            TRG_LEVEL       = 3
        };
};

/*
 * APIC Structure (5.2.11.4)
 */
class Acpi_apic
{
    public:
        uint8   type;
        uint8   length;

        enum Type
        {
            LAPIC   = 0,
            IOAPIC  = 1,
            INTR    = 2,
        };
};

/*
 * Processor Local APIC (5.2.11.5)
 */
class Acpi_lapic : public Acpi_apic
{
    public:
        uint8   cpu;
        uint8   id;
        uint32  flags;
};

/*
 * I/O APIC (5.2.11.6)
 */
class Acpi_ioapic : public Acpi_apic
{
    public:
        uint8   id;
        uint8   zero;
        uint32  phys;
        uint32  gsi;
};

/*
 * Interrupt Source Override (5.2.11.8)
 */
class Acpi_intr : public Acpi_apic
{
    public:
        uint8       bus;
        uint8       irq;
        uint32      gsi;
        Acpi_inti   flags;
};

/*
 * Multiple APIC Description Table
 */
class Acpi_table_madt : public Acpi_table
{
    private:
        INIT
        static void parse_lapic (Acpi_apic const *);

        INIT
        static void parse_ioapic (Acpi_apic const *);

        INIT
        void parse_entry (Acpi_apic::Type, void (*)(Acpi_apic const *)) const;

    public:
        uint32      apic_addr;
        uint32      flags;
        Acpi_apic   apic[];

        static bool sci_overridden;

        INIT
        void parse() const;

        INIT
        static void parse_intr (Acpi_apic const *);
};

#pragma pack()
