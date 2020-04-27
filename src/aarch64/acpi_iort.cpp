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

#include "acpi_iort.hpp"

void Acpi_table_iort::parse() const
{
    for (auto d = reinterpret_cast<uint8 const *>(this) + node_ofs; d < reinterpret_cast<uint8 const *>(this) + length; ) {

        auto n = reinterpret_cast<Node const *>(d);

        d += n->length;
    }
}
