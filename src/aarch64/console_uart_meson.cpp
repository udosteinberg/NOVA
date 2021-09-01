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

#include "console_uart_meson.hpp"

INIT_PRIORITY (PRIO_CONSOLE) Console_uart_meson Console_uart_meson::singleton { (Board::uart.type == Debug::Subtype::SERIAL_MESON) * Board::uart.mmio, Board::uart.clock };
