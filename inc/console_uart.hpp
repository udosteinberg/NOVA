/*
 * UART Console
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

#include "console.hpp"
#include "lowlevel.hpp"

template <typename U>
class Console_uart : protected Console
{
    private:
        bool fini() override final
        {
            auto uart = static_cast<U *>(this);

            while (EXPECT_FALSE (uart->tx_busy()))
                pause();

            return true;
        }

        void outc (char c) override final
        {
            auto uart = static_cast<U *>(this);

            while (EXPECT_FALSE (uart->tx_full()))
                pause();

            uart->tx (c);
        }

    protected:
        static unsigned const baudrate = 115200;
};
