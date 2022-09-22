/*
 * Console: Super I/O (SIO) UART
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

#include "console_uart_ns16550.hpp"

class Console_uart_sio final : private Console_uart_ns16550
{
    private:
        static Console_uart_sio uart[];

        static constexpr port_t pio[8] { 0x3f8, 0x2f8, 0x220, 0x228, 0x238, 0x2e8, 0x338, 0x3e8 };

        static Regs probe (pci_t, unsigned, unsigned);

    protected:
        explicit Console_uart_sio (pci_t pci, unsigned e, unsigned s) : Console_uart_ns16550 { probe (pci, e, s), 1'843'200 } {}
};
