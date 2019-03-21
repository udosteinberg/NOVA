/*
 * Console: Serial Communication Interface with FIFO (SCIF)
 *
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "console_uart_mmio.hpp"

class Console_scif_mmio
{
    public:
        enum class Register
        {
            SCSMR       = 0x00,     // Serial Mode Register
            SCBRR       = 0x04,     // Bit Rate Register
            SCSCR       = 0x08,     // Serial Control Register
            SCFTDR      = 0x0c,     // FIFO Transmit Data Register
            SCFSR       = 0x10,     // FIFO Status Register
            SCFRDR      = 0x14,     // FIFO Receive Data Register
            SCFCR       = 0x18,     // FIFO Control Register
            SCFDR       = 0x1c,     // FIFO Data Count Register
            SCSPTR      = 0x20,     // Serial Port Register
            SCLSR       = 0x24,     // Line Status Register
            DL          = 0x30,     // Frequency Division Register
            CKS         = 0x34,     // Clock Select Register
        };
};

class Console_scif final : private Console_scif_mmio, private Console_uart_mmio<Console_scif, Console_scif_mmio::Register>
{
    friend class Console_uart;
    friend class Console_uart_mmio;

    private:
        static Console_scif con;

        bool tx_busy() { return !(read<uint16> (Register::SCFSR) & BIT (6)); }
        bool tx_full() { return !(read<uint16> (Register::SCFSR) & BIT (5)); }

        void tx (uint8 c)
        {
            write<uint8>  (Register::SCFTDR, c);
            write<uint16> (Register::SCFSR,  static_cast<uint16>(~BIT_RANGE (6, 5)) & read<uint16>(Register::SCFSR));
        }

        void init()
        {
            write<uint16> (Register::SCSCR, 0);
            write<uint16> (Register::SCFCR, BIT_RANGE (2, 1));
            write<uint16> (Register::SCSMR, 0);
            write<uint16> (Register::SCFCR, 0);
            write<uint16> (Register::SCSCR, BIT (5));
        }

        Console_scif (Hpt::OAddr p) : Console_uart_mmio (p) {}
};
