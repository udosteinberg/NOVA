/*
 * Advanced Configuration and Power Interface (ACPI)
 *
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

#include "types.hpp"

/*
 * 5.2.10: Firmware ACPI Control Structure (FACS)
 */
class Acpi_table_facs
{
    private:
        uint32      signature;                      //  0
        uint32      length;                         //  4
        uint32      hw_signature;                   //  8
        uint32      fw_waking_vector_32;            // 12
        uint32      global_lock;                    // 16
        uint32      flags;                          // 20
        uint64      fw_waking_vector_64;            // 24
        uint8       version;                        // 32
        uint8       reserved1[3];                   // 33
        uint32      ospm_flags;                     // 36
        uint8       reserved2[24];                  // 40
};
