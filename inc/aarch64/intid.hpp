/*
 * Interrupt Identifier
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

class Intid
{
    protected:
        static constexpr unsigned BASE_SGI  {    0 };
        static constexpr unsigned BASE_PPI  {   16 };
        static constexpr unsigned BASE_SPI  {   32 };
        static constexpr unsigned BASE_RES1 { 1020 };   // Reserved
        static constexpr unsigned BASE_EPPI { 1056 };
        static constexpr unsigned BASE_RES2 { 1120 };   // Reserved
        static constexpr unsigned BASE_ESPI { 4096 };
        static constexpr unsigned BASE_RES3 { 5120 };   // Reserved
        static constexpr unsigned BASE_LPI  { 8192 };

    public:
        enum class Type : unsigned
        {
            SGI,
            PPI,
            SPI,
            EPPI,
            ESPI,
            LPI,
            UNKNOWN,
        };

        static constexpr auto type (unsigned id)
        {
            if (id < BASE_PPI)  return Type::SGI;       //    0 ...   15
            if (id < BASE_SPI)  return Type::PPI;       //   16 ...   31
            if (id < BASE_RES1) return Type::SPI;       //   32 ... 1019
            if (id < BASE_EPPI) return Type::UNKNOWN;   // 1020 ... 1055
            if (id < BASE_RES2) return Type::EPPI;      // 1056 ... 1119
            if (id < BASE_ESPI) return Type::UNKNOWN;   // 1120 ... 4095
            if (id < BASE_RES3) return Type::ESPI;      // 4096 ... 5119
            if (id < BASE_LPI)  return Type::UNKNOWN;   // 5120 ... 8191

            return Type::LPI;                           // 8192 ...
        }

        static constexpr unsigned NUM_SGI   { BASE_PPI  - BASE_SGI  };  //   16
        static constexpr unsigned NUM_PPI   { BASE_SPI  - BASE_PPI  };  //   16
        static constexpr unsigned NUM_SPI   { BASE_RES1 - BASE_SPI  };  //  988
        static constexpr unsigned NUM_EPPI  { BASE_RES2 - BASE_EPPI };  //   64
        static constexpr unsigned NUM_ESPI  { BASE_RES3 - BASE_ESPI };  // 1024

        // Convert INTID to SGI/PPI/SPI/LPI number
        static constexpr auto to_sgi  (unsigned id) { return id - BASE_SGI;  }
        static constexpr auto to_ppi  (unsigned id) { return id - BASE_PPI;  }
        static constexpr auto to_spi  (unsigned id) { return id - BASE_SPI;  }
        static constexpr auto to_eppi (unsigned id) { return id - BASE_EPPI; }
        static constexpr auto to_espi (unsigned id) { return id - BASE_ESPI; }
        static constexpr auto to_lpi  (unsigned id) { return id - BASE_LPI;  }

        // Convert SGI/PPI/SPI/LPI number to INTID
        static constexpr auto from_sgi  (unsigned n) { return n + BASE_SGI;  }
        static constexpr auto from_ppi  (unsigned n) { return n + BASE_PPI;  }
        static constexpr auto from_spi  (unsigned n) { return n + BASE_SPI;  }
        static constexpr auto from_eppi (unsigned n) { return n + BASE_EPPI; }
        static constexpr auto from_espi (unsigned n) { return n + BASE_ESPI; }
        static constexpr auto from_lpi  (unsigned n) { return n + BASE_LPI;  }
};
