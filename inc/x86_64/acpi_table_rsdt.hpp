/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

/*
 * 5.2.7: Root System Description Table (RSDT)
 * 5.2.8: Extended System Description Table (XSDT)
 */
class Acpi_table_rsdt : private Acpi_table
{
    private:
        uint32  rsdt[1];

        auto count (size_t size) const { return (length - sizeof (Acpi_table)) / size; }

        auto entry (unsigned i) const { return __atomic_load_n (rsdt + i, __ATOMIC_RELAXED); }

    public:
        void parse (uint64, size_t) const;
};
