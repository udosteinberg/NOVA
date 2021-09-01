/*
 * Console: Meson UART
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

class Console_uart_meson final : private Console_uart
{
    private:
        static Console_uart_meson singleton;

        enum class Register32 : unsigned
        {
            WBUF        = 0x00,     // Write Buffer Register
            RBUF        = 0x04,     // Read Buffer Register
            CTRL        = 0x08,     // Control Register
            STAT        = 0x0c,     // Status Register
            MISC        = 0x10,     // Interrupt Control Register
            BAUD        = 0x14,     // New Baud Rate Register
        };

        auto read  (Register32 r) const      { return *reinterpret_cast<uint32_t volatile *>(mmap + std::to_underlying (r)); }
        void write (Register32 r, uint32_t v) const { *reinterpret_cast<uint32_t volatile *>(mmap + std::to_underlying (r)) = v; }

        bool tx_busy() const override final { return read (Register32::STAT) & BIT (25); }
        bool tx_full() const override final { return read (Register32::STAT) & BIT (21); }
        void tx (uint8_t c) const override final { write (Register32::WBUF, c); }

        void init() const override final
        {
            if (!clock)             // Keep the existing settings
                return;

            write (Register32::CTRL, BIT_RANGE (24, 22));
            write (Register32::MISC, 0);
            write (Register32::BAUD, BIT_RANGE (24, 23) | (clock / 3 / baudrate & BIT_RANGE (22, 0)));
            write (Register32::CTRL, BIT (15) | BIT (12));
        }

    protected:
        explicit Console_uart_meson (Hpt::OAddr p, unsigned c) : Console_uart (c) { setup_regs (Acpi_gas::Asid::MMIO, p); }
};
