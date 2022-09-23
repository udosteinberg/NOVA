/*
 * Console: Serial-Over-LAN (SOL)
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

#include "console_uart_ns16550.hpp"

class Console_uart_sol final : private Console_uart_ns16550
{
    private:
        static Console_uart_sol uart0;

        static Regs probe (pci_t);

    protected:
        explicit Console_uart_sol (pci_t bdf) : Console_uart_ns16550 (probe (bdf), 0) {}
};
