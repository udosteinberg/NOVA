/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "acpi_table_rsdp.hpp"
#include "compiler.hpp"

bool Acpi_table_rsdp::parse (uint64_t &addr, size_t &size) const
{
    if (EXPECT_FALSE (!valid()))
        return false;

    addr = revision > 1 ? static_cast<uint64_t>(xsdt_addr_hi) << 32 | xsdt_addr_lo : rsdt_addr;
    size = revision > 1 ? sizeof (uint64_t) : sizeof (uint32_t);

    return true;
}
