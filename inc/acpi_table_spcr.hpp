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

#include "acpi_gas.hpp"
#include "acpi_table_dbg2.hpp"

/*
 * Serial Port Console Redirection Table (SPCR)
 */
class Acpi_table_spcr final
{
    // Note that DBG2 extends this to a 16bit value
    using Subtype = Acpi_table_dbg2::Info::Subtype;

    private:
        Acpi_table  table;                          //   0
        Subtype     uart_type;                      //  36
        uint16      reserved;                       //  38
        Acpi_gas    regs;                           //  40
        uint8       intr_type;                      //  52
        uint8       irq;                            //  53
        uint8       gsi[4];                         //  54
        uint8       baud_rate;                      //  58
        uint8       parity;                         //  59
        uint8       stop_bits;                      //  60
        uint8       flow_ctrl;                      //  61
        uint8       term_type;                      //  62
        uint8       language;                       //  63
        uint16      pci_did;                        //  64
        uint16      pci_vid;                        //  66

    public:
        void parse() const;
};

static_assert (__is_standard_layout (Acpi_table_spcr) && sizeof (Acpi_table_spcr) == 68);
