/*
 * UART Console (MMIO)
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

#include "console_uart.hpp"
#include "hpt.hpp"
#include "memory.hpp"

template <typename U, typename R>
class Console_uart_mmio : protected Console_uart<U>
{
    protected:
        template <typename T> inline auto read  (R r) { return *reinterpret_cast<T volatile *>(DEV_GLOBL_UART + static_cast<mword>(r)); }
        template <typename T> inline void write (R r, T v)   { *reinterpret_cast<T volatile *>(DEV_GLOBL_UART + static_cast<mword>(r)) = v; }

        Console_uart_mmio (Hpt::OAddr phys)
        {
            if (!phys)
                return;

            Hptp::master.update (DEV_GLOBL_UART, phys, 0, Paging::Permissions (Paging::R | Paging::W | Paging::G), Memattr::Cacheability::DEV, Memattr::Shareability::NONE);

            static_cast<U *>(this)->init();

            Console::enable();
        }
};
