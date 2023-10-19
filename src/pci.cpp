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
#include "ptab_hpt.hpp"
#include "stdio.hpp"
#include "util.hpp"

INIT_PRIORITY (PRIO_SLAB) Slab_cache Pci::Device::cache { sizeof (Pci::Device), alignof (Pci::Device) };

Pci::Device::Device (pci_t p, uint8_t l) : List { list }, pci { p }, lev { l }
{
    enumerate_pcap();
    enumerate_ecap();

    auto const didvid { read (Cfg::Reg32::DID_VID) };
    auto const ccprid { read (Cfg::Reg32::CCP_RID) };

    auto const flr { cap<Cap_pcie>() ? !!(read<Cap_pcie>(Cap_pcie::Reg32::DCAP) & BIT (28)) : false };
    auto const pms { cap<Cap_pmi>() ? read<Cap_pmi>(Cap_pmi::Reg32::PMCSR) & BIT_RANGE (1, 0) : 0 };

    trace (TRACE_PCI, "PCIE: %04x:%04x %02x-%02x-%02x D%u %4s %3s %3s %5s %*s%04x:%02x:%02x.%x",
           static_cast<uint16_t>(didvid), static_cast<uint16_t>(didvid >> 16),
           static_cast<uint8_t>(ccprid >> 24), static_cast<uint8_t>(ccprid >> 16), static_cast<uint8_t>(ccprid >> 8),
           pms, cap<Cap_pcie>() ? "PCIE" : cap<Cap_pcix>() ? "PCIX" : "",
           cap<Cap_pmi>() ? "PMI" : "", flr ? "FLR" : "", cap<Cap_sriov>() ? "SRIOV" : "",
           3 * lev, "", seg (pci), bus (pci), dev (pci), fun (pci));
}

void Pci::Device::enumerate_pcap()
{
    if (EXPECT_FALSE (!(read (Cfg::Reg16::STS) & BIT (4))))
        return;

    uint8_t ptr { read (Cfg::Reg8::CAP) };

    // Software must mask the bottom two bits of the 8-bit pointer
    for (uint16_t cap; ptr &= BIT_RANGE (7, 2); ptr = static_cast<uint8_t>(cap >> 8))
        switch (static_cast<Pcap::Type>(cap = read (Cfg::Reg16 { ptr }))) {
            case Pcap::Type::PMI:  static_cast<Cap_pmi  *>(this)->ptr = ptr; break;
            case Pcap::Type::PCIX: static_cast<Cap_pcix *>(this)->ptr = ptr; break;
            case Pcap::Type::PCIE: static_cast<Cap_pcie *>(this)->ptr = ptr; break;
            default: break;
        }
}

void Pci::Device::enumerate_ecap()
{
    if (EXPECT_FALSE (!cap<Cap_pcie>()))
        return;

    uint16_t ptr { 0x100 };

    // Software must mask the bottom two bits of the 12-bit pointer
    for (uint32_t cap; ptr &= BIT_RANGE (11, 2); ptr = static_cast<uint16_t>(cap >> 20))
        switch (static_cast<Ecap::Type>(cap = read (Cfg::Reg32 { ptr }))) {
            case Ecap::Type::SRIOV: static_cast<Cap_sriov *>(this)->ptr = ptr; break;
            default: break;
        }
}

uint8_t Pci::init_bus (uint16_t const seg, uint8_t const bus, uint8_t const ebn, uint8_t const lev)
{
    if (EXPECT_FALSE (bus > ebn))
        return bus;

    auto hbn { bus };

    for (unsigned i { 0 }; i < 256; i++) {

        auto const pci { static_cast<pci_t>(seg << seg_shft | bus << bus_shft | i) };

        if (*reinterpret_cast<uint32_t volatile *>(ecam_addr (pci)) == BIT_RANGE (31, 0))
            continue;

        auto const dev { new Device { pci, lev } };
        auto const hdr { dev->read (Cfg::Reg8::HDR) };

        // PCI-PCI Bridge
        if ((hdr & BIT_RANGE (6, 0)) == 1) {
            auto const num { dev->read (Cfg::Reg32::BUS_NUM) };
            auto const sec { static_cast<uint8_t>(num >>  8) };
            auto const sub { static_cast<uint8_t>(num >> 16) };
            init_bus (seg, sec, ebn, lev + 1);
            hbn = max (sub, hbn);
        }

        // Multi-Function Device
        if (!fun (pci) && !(hdr & BIT (7)))
            i += 7;
    }

    return hbn;
}

bool Pci::init_seg (uint64_t phys, uint16_t const seg, uint8_t const sbn, uint8_t const ebn)
{
    if (EXPECT_FALSE (seg >= seg_grps))
        return false;

    auto virt { ecam_addr (seg << seg_shft) };
    auto size { static_cast<uint64_t>(ebn + 1) * (cfg_size << bus_shft) };

    // PCIE Specification: Segment groups must be at least 2MiB aligned
    if ((virt | phys) & Hpt::offs_mask (Hpt::bpl))
        return false;

    trace (TRACE_FIRM, "PCIE: %#010lx Segment %#06x Bus %#04x-%#04x", phys, seg, sbn, ebn);

    for (unsigned o; size; size -= BITN (o), phys += BITN (o), virt += BITN (o))
        Hptp::master_map (virt, phys, (o = static_cast<unsigned>(min (max_order (phys, size), max_order (virt, size)))) - PAGE_BITS, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev());

    for (auto bus { sbn }; (bus = init_bus (seg, bus, ebn, 0)) < ebn; bus++) ;

    return true;
}
