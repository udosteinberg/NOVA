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
        virtual bool tx_busy() { return false; }
        virtual bool tx_full() { return false; }

        virtual void tx (uint8) {}

        bool fini() override final
        {
            while (EXPECT_FALSE (tx_busy()))
                pause();

            return true;
        }

        void outc (char c) override final
        {
            while (EXPECT_FALSE (tx_full()))
                pause();

            tx (c);
        }

    protected:
        static unsigned const baudrate = 115200;
};
