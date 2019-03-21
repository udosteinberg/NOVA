/*
 * Console: Serial Communication Interface with FIFO (SCIF)
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

#include "console_uart.hpp"

class Console_uart_scif final : private Console_uart
{
    private:
        static Console_uart_scif singleton;

        enum class Reg8 : unsigned
        {
            SCBRR       = 0x04,     // Bit Rate Register
            SCFTDR      = 0x0c,     // FIFO Transmit Data Register
            SCFRDR      = 0x14,     // FIFO Receive Data Register
        };

        enum class Reg16 : unsigned
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

        auto read  (Reg8  r) const      { return *reinterpret_cast<uint8_t  volatile *>(mmap + std::to_underlying (r)); }
        auto read  (Reg16 r) const      { return *reinterpret_cast<uint16_t volatile *>(mmap + std::to_underlying (r)); }
        void write (Reg8  r, uint8_t  v) const { *reinterpret_cast<uint8_t  volatile *>(mmap + std::to_underlying (r)) = v; }
        void write (Reg16 r, uint16_t v) const { *reinterpret_cast<uint16_t volatile *>(mmap + std::to_underlying (r)) = v; }

        bool tx_busy() const override final { return !(read (Reg16::SCFSR) & BIT (6)); }
        bool tx_full() const override final { return !(read (Reg16::SCFSR) & BIT (5)); }

        inline void tx (uint8_t c) const override final
        {
            write (Reg8::SCFTDR, c);
            write (Reg16::SCFSR, uint16_t (~BIT_RANGE (6, 5)) & read (Reg16::SCFSR));
        }

        bool init() const override final
        {
            write (Reg16::SCSCR, 0);
            write (Reg16::SCFCR, BIT_RANGE (2, 1));
            write (Reg16::SCSMR, 0);
            write (Reg16::SCFCR, 0);
            write (Reg16::SCSCR, BIT (5));

            return true;
        }

    protected:
        explicit Console_uart_scif (Hpt::OAddr p, unsigned c) : Console_uart (c) { setup_regs (Acpi_gas::Asid::MMIO, p); }
};
