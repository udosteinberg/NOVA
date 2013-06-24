/*
 * Advanced Configuration and Power Interface (ACPI)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#include "acpi_gas.hpp"
#include "acpi_table.hpp"

#pragma pack(1)

class Acpi_table_hpet : public Acpi_table
{
    public:
        uint32      cap;
        Acpi_gas    hpet;
        uint8       id;
        uint16      tick;

        INIT
        void parse() const;
};

#pragma pack()
