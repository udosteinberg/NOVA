/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Author: Udo Steinberg <udo@hypervisor.org>
 * TU Dresden, Operating Systems Group
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

#include "acpi_table.h"

/*
 * Device Scope Structure
 */
class Acpi_scope
{
    public:
        uint8       type;
        uint8       length;
        uint16      reserved;
        uint8       id, b, d, f;
};

/*
 * Remapping Structure
 */
class Acpi_remap
{
    public:
        uint16      type;
        uint16      length;

        enum Type
        {
            DMAR    = 0,
            RMRR    = 1,
            ATSR    = 2
        };
};

/*
 * DMA Remapping Hardware Unit Definition Structure
 */
class Acpi_dmar : public Acpi_remap
{
    public:
        uint8       flags;
        uint8       reserved;
        uint16      segment;
        uint64      phys;
        Acpi_scope  scope[];

        INIT
        void parse() const;
};

/*
 * Reserved Memory Region Reporting Structure
 */
class Acpi_rmrr : public Acpi_remap
{
    public:
        uint16      reserved;
        uint16      segment;
        uint64      base;
        uint64      limit;
        Acpi_scope  scope[];

        INIT
        void parse() const;
};

/*
 * Root Port ATS Capability Reporting Structure
 */
class Acpi_atsr : public Acpi_remap
{
    public:
        uint8       flags;
        uint8       reserved;
        uint16      segment;
        Acpi_scope  scope[];

        INIT
        void parse() const;
};

/*
 * DMA Remapping Reporting Structure
 */
class Acpi_table_dmar : public Acpi_table
{
    public:
        uint8       haw;
        uint8       flags;
        uint8       reserved[10];
        Acpi_remap  remap[];

        INIT
        void parse() const;
};
