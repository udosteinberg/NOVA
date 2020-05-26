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

class Console_uart : protected Console
{
    private:
        virtual bool tx_busy()  = 0;
        virtual bool tx_full()  = 0;
        virtual void tx (uint8) = 0;

        inline bool fini() override final
        {
            while (EXPECT_FALSE (tx_busy()))
                pause();

            return true;
        }

        inline void outc (char c) override final
        {
            while (EXPECT_FALSE (tx_full()))
                pause();

            tx (c);
        }

    protected:
        static constexpr unsigned baudrate = 115200;

        unsigned const clock;

        virtual void init() = 0;

        inline explicit Console_uart (unsigned c) : clock (c) {}
};
