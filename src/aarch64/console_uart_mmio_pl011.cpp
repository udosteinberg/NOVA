/*
 * Console: PrimeCell UART (PL011)
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

#include "console_uart_mmio_pl011.hpp"

INIT_PRIORITY (PRIO_CONSOLE) Console_uart_mmio_pl011 Console_uart_mmio_pl011::singleton ((Board::uart.type == Uart_type::PL011) * Board::uart.mmio, Board::uart.clock);
