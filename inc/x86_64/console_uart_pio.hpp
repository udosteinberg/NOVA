/*
 * UART Console (Port I/O)
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
#include "io.hpp"

template <typename U, typename R>
class Console_uart_pio : protected Console_uart<U>
{
    protected:
        unsigned base;

        template <typename T> inline auto in  (R r) { return Io::in <T>(base + static_cast<unsigned>(r)); }
        template <typename T> inline void out (R r, T v)   { Io::out<T>(base + static_cast<unsigned>(r), v); }
};
