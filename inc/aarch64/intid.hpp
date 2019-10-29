/*
 * Interrupt Identifier
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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

class Intid
{
    protected:
        static constexpr unsigned BASE_SGI {    0 };
        static constexpr unsigned BASE_PPI {   16 };
        static constexpr unsigned BASE_SPI {   32 };
        static constexpr unsigned BASE_RSV { 1020 };

    public:
        static constexpr unsigned NUM_SGI { BASE_PPI - BASE_SGI };
        static constexpr unsigned NUM_PPI { BASE_SPI - BASE_PPI };
        static constexpr unsigned NUM_SPI { BASE_RSV - BASE_SPI };

        static inline constexpr auto to_sgi (unsigned id) { return id - BASE_SGI; }
        static inline constexpr auto to_ppi (unsigned id) { return id - BASE_PPI; }
        static inline constexpr auto to_spi (unsigned id) { return id - BASE_SPI; }

        static inline constexpr auto from_sgi (unsigned sgi) { return sgi + BASE_SGI; }
        static inline constexpr auto from_ppi (unsigned ppi) { return ppi + BASE_PPI; }
        static inline constexpr auto from_spi (unsigned spi) { return spi + BASE_SPI; }
};
