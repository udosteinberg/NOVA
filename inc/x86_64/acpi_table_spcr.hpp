/*
 * Advanced Configuration and Power Interface (ACPI)
 *
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

#pragma once

#include "acpi_gas.hpp"
#include "acpi_table.hpp"
#include "debug.hpp"

/*
 * Serial Port Console Redirection Table (SPCR)
 */
class Acpi_table_spcr final
{
    private:
        Acpi_table      table;                      //   0
        Debug::Subtype  uart_type;                  //  36
        uint16_t        reserved;                   //  38
        Acpi_gas        regs;                       //  40
        uint8_t         intr_type;                  //  52
        uint8_t         irq;                        //  53
        uint8_t         gsi[4];                     //  54
        uint8_t         baud_rate;                  //  58
        uint8_t         parity;                     //  59
        uint8_t         stop_bits;                  //  60
        uint8_t         flow_ctrl;                  //  61
        uint8_t         term_type;                  //  62
        uint8_t         language;                   //  63
        uint16_t        pci_did;                    //  64
        uint16_t        pci_vid;                    //  66

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_spcr) && sizeof (Acpi_table_spcr) == 68);
