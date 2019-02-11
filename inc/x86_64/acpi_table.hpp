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

#include "compiler.hpp"
#include "types.hpp"

#define SIG(A,B,C,D) (A + (B << 8) + (C << 16) + (D << 24))

class Acpi_table
{
    public:
        uint32      signature;                      // 0
        uint32      length;                         // 4
        uint8       revision;                       // 8
        uint8       checksum;                       // 9
        char        oem_id[6];                      // 10
        char        oem_table_id[8];                // 16
        uint32      oem_revision;                   // 24
        char        creator_id[4];                  // 28
        uint32      creator_revision;               // 32

        INIT
        bool good_checksum (Paddr addr) const;
};
