/*
 * PCI Configuration Space
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

#include "pci.hpp"
#include "stdio.hpp"
#include "util.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Pci::cache { sizeof (Pci), alignof (Pci) };

Pci::Pci (pci_t s, uint16_t l) : List (list), sid (s), lev (l)
{
    auto const didvid { cfg_read (Register32::DID_VID) };

    trace (TRACE_PCI, "PCIE: Device %04x:%04x (%#010x) %*s%02x:%02x.%x", static_cast<uint16_t>(didvid), static_cast<uint16_t>(didvid >> 16), cfg_read (Register32::CCP_RID), 3 * lev, "", bus (sid), dev (sid), fun (sid));

    for (unsigned i { 0 }; i < sizeof quirks / sizeof *quirks; i++)
        if (quirks[i].didvid == didvid)
            (this->*quirks[i].func)();
}

uint8_t Pci::init (uint8_t const bus, uint8_t const ebn, uint16_t const lev)
{
    if (EXPECT_FALSE (bus > ebn))
        return bus;

    auto hbn { bus };

    for (unsigned i { 0 }; i < 256; i++) {

        auto const sid { static_cast<pci_t>(bus << 8 | i) };

        if (*reinterpret_cast<uint32_t volatile *>(ecam_addr (sid)) == BIT_RANGE (31, 0))
            continue;

        auto const pci { new Pci (sid, lev) };
        auto const hdr { pci->cfg_read (Register8::HTYPE) };

        // PCI-PCI Bridge
        if ((hdr & BIT_RANGE (6, 0)) == 1) {
            init (pci->cfg_read (Register8::BUS_SEC), ebn, lev + 1);
            hbn = max (pci->cfg_read (Register8::BUS_SUB), hbn);
        }

        // Multi-Function Device
        if (!fun (sid) && !(hdr & BIT (7)))
            i += 7;
    }

    return hbn;
}
