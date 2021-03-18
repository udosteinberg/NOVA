/*
 * PCI Configuration Space: Architecture-Specific (x86)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#pragma once

#include "io.hpp"
#include "pci.hpp"

class Pci_arch final
{
    private:
        // Configuration Space Address
        static void addr (pci_t p, unsigned r) { Io::out<uint32_t>(0xcf8, BIT (31) | Pci::bdf (p) << 8 | r); }

        // Configuration Space Data
        static constexpr auto data (Pci::Cfg::Reg8  r) { return static_cast<port_t>(0xcfc | (std::to_underlying (r) & 3)); }
        static constexpr auto data (Pci::Cfg::Reg16 r) { return static_cast<port_t>(0xcfc | (std::to_underlying (r) & 2)); }
        static constexpr auto data (Pci::Cfg::Reg32)   { return static_cast<port_t>(0xcfc); }

    public:
        // Configuration Space Read
        static auto read (pci_t p, Pci::Cfg::Reg8  r) { addr (p, std::to_underlying (r)); return Io::in<uint8_t> (data (r)); }
        static auto read (pci_t p, Pci::Cfg::Reg16 r) { addr (p, std::to_underlying (r)); return Io::in<uint16_t>(data (r)); }
        static auto read (pci_t p, Pci::Cfg::Reg32 r) { addr (p, std::to_underlying (r)); return Io::in<uint32_t>(data (r)); }

        // Configuration Space Write
        static void write (pci_t p, Pci::Cfg::Reg8  r, uint8_t  v) { addr (p, std::to_underlying (r)); Io::out<uint8_t> (data (r), v); }
        static void write (pci_t p, Pci::Cfg::Reg16 r, uint16_t v) { addr (p, std::to_underlying (r)); Io::out<uint16_t>(data (r), v); }
        static void write (pci_t p, Pci::Cfg::Reg32 r, uint32_t v) { addr (p, std::to_underlying (r)); Io::out<uint32_t>(data (r), v); }

        // Configuration Space Read/Write
        static void rd_wr (pci_t p, Pci::Cfg::Reg8  r, uint8_t  s) { addr (p, std::to_underlying (r)); Io::out<uint8_t> (data (r), Io::in<uint8_t> (data (r)) | s); }
        static void rd_wr (pci_t p, Pci::Cfg::Reg16 r, uint16_t s) { addr (p, std::to_underlying (r)); Io::out<uint16_t>(data (r), Io::in<uint16_t>(data (r)) | s); }
        static void rd_wr (pci_t p, Pci::Cfg::Reg32 r, uint32_t s) { addr (p, std::to_underlying (r)); Io::out<uint32_t>(data (r), Io::in<uint32_t>(data (r)) | s); }
};
