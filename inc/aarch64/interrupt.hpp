/*
 * Interrupt Handling
 *
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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

#include "intid.hpp"

class Interrupt final : private Intid
{
    public:
        static inline constinit unsigned num_spi    { 0 };
        static inline constinit unsigned num_eppi   { 0 };
        static inline constinit unsigned num_espi   { 0 };
        static inline constinit unsigned num_lpi    { 0 };

        enum Request
        {
            RRQ,
            RKE,
        };
};
