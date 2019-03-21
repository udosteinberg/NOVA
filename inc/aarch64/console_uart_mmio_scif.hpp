/*
 * Console: Serial Communication Interface with FIFO (SCIF)
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

#pragma once

#include "console_uart_mmio.hpp"

class Console_uart_mmio_scif final : private Console_uart_mmio
{
    private:
        static Console_uart_mmio_scif singleton;

        enum class Register8
        {
            SCBRR       = 0x04,     // Bit Rate Register
            SCFTDR      = 0x0c,     // FIFO Transmit Data Register
            SCFRDR      = 0x14,     // FIFO Receive Data Register
        };

        enum class Register16
        {
            SCSMR       = 0x00,     // Serial Mode Register
            SCSCR       = 0x08,     // Serial Control Register
            SCFSR       = 0x10,     // FIFO Status Register
            SCFCR       = 0x18,     // FIFO Control Register
            SCFDR       = 0x1c,     // FIFO Data Count Register
            SCSPTR      = 0x20,     // Serial Port Register
            SCLSR       = 0x24,     // Line Status Register
            DL          = 0x30,     // Frequency Division Register
            CKS         = 0x34,     // Clock Select Register
        };

        inline auto read  (Register8  r)    { return *reinterpret_cast<uint8  volatile *>(mmio_base + static_cast<unsigned>(r)); }
        inline auto read  (Register16 r)    { return *reinterpret_cast<uint16 volatile *>(mmio_base + static_cast<unsigned>(r)); }
        inline void write (Register8  r, uint8  v) { *reinterpret_cast<uint8  volatile *>(mmio_base + static_cast<unsigned>(r)) = v; }
        inline void write (Register16 r, uint16 v) { *reinterpret_cast<uint16 volatile *>(mmio_base + static_cast<unsigned>(r)) = v; }

        inline bool tx_busy() override final { return !(read (Register16::SCFSR) & BIT (6)); }
        inline bool tx_full() override final { return !(read (Register16::SCFSR) & BIT (5)); }

        inline void tx (uint8 c) override final
        {
            write (Register8::SCFTDR, c);
            write (Register16::SCFSR, uint16 (~BIT_RANGE (6, 5)) & read (Register16::SCFSR));
        }

        void init()
        {
            write (Register16::SCSCR, 0);
            write (Register16::SCFCR, BIT_RANGE (2, 1));
            write (Register16::SCSMR, 0);
            write (Register16::SCFCR, 0);
            write (Register16::SCSCR, BIT (5));
        }

    public:
        Console_uart_mmio_scif (Hpt::OAddr p, unsigned c) : Console_uart_mmio (p, c)
        {
            if (!mmio_base)
                return;

            init();

            enable();
        }
};
