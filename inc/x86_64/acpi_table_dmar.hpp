/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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
#include "compiler.hpp"

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

        ALWAYS_INLINE
        inline unsigned rid() const { return b << 8 | d << 3 | f; }
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

        void parse() const;
};
