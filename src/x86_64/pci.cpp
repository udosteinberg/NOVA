/*
 * PCI Configuration Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

INIT_PRIORITY (PRIO_SLAB) Slab_cache Pci::Function::cache { sizeof (Pci::Function), alignof (Pci::Function) };

Pci::Function::Function (pci_t s, uint8_t l) : List { list }, sbdf { s }, lev { l }
{
    enumerate_pcap();
    enumerate_ecap();

    auto const didvid { read (Cfg::Reg32::DID_VID) };
    auto const ccprid { read (Cfg::Reg32::CCP_RID) };

    auto const flr { cap<Cap_pcie>() ? !!(read (Cap_pcie::Reg32::DCAP) & BIT (28)) : false };
    auto const pms { cap<Cap_pmi>() ? read (Cap_pmi::Reg32::PMCSR) & BIT_RANGE (1, 0) : 0 };

    trace (TRACE_PCI, "PCIE: %04x:%04x %02x-%02x-%02x D%u %3s %3s %4s %3s %4s %5s %3s %3s %*s%04x:%02x:%02x.%x",
           static_cast<uint16_t>(didvid), static_cast<uint16_t>(didvid >> 16),
           static_cast<uint8_t>(ccprid >> 24), static_cast<uint8_t>(ccprid >> 16), static_cast<uint8_t>(ccprid >> 8),
           pms, flr ? "FLR" : "",
           cap<Cap_pmi>() ? "PMI" : "", cap<Cap_pcie>() ? "PCIE" : "", cap<Cap_msi>() ? "MSI" : "",
           cap<Cap_msix>() ? "MSIX" : "", cap<Cap_sdev>() ? "SDEV" : cap<Cap_sriov>() ? "SRIOV" : "",
           cap<Cap_ats>() ? "ATS" : "", cap<Cap_pri>() ? "PRI" : "",
           3 * lev, "", seg (sbdf), bus (sbdf), dev (sbdf), fun (sbdf));
}

void Pci::Function::enumerate_pcap()
{
    constexpr auto A { 2 }, B { 8 }, O { 0x40 };

    // No capabilities to enumerate
    if (!(read (Cfg::Reg16::STS) & BIT (4))) [[unlikely]]
        return;

    // Because a malicious device could implement a circular capability list, the worst-case number of PCAPs serves as a limit
    unsigned lim { (BIT (B) - O) >> A };

    // Because capabilities are 32-bit aligned, bits[1:0] of the 8-bit pointer must be masked and the result must be a valid offset or 0
    for (uint8_t ptr { read (Cfg::Reg8::CAP) }; lim-- && (ptr &= BIT_RANGE (B - 1, A)) >= O; ) {

        auto const cap { read (Cfg::Reg16 { ptr }) };

        // Bail out for (emulated) devices that implement PCAPs incorrectly
        if (cap == BIT_RANGE (15, 0)) [[unlikely]]
            return;

        // Store capability offsets
        switch (static_cast<Pcap::Type>(cap)) {
            case Pcap::Type::PMI:  static_cast<Cap_pmi  *>(this)->off = ptr; break;
            case Pcap::Type::PCIE: static_cast<Cap_pcie *>(this)->off = ptr; break;
            case Pcap::Type::MSI:  static_cast<Cap_msi  *>(this)->off = ptr; break;
            case Pcap::Type::MSIX: static_cast<Cap_msix *>(this)->off = ptr; break;
            case Pcap::Type::SDEV: static_cast<Cap_sdev *>(this)->off = ptr; break;
            default: break;
        }

        ptr = static_cast<uint8_t>(cap >> 8);
    }
}

void Pci::Function::enumerate_ecap()
{
    constexpr auto A { 2 }, B { 12 }, O { 0x100 };

    // No capabilities to enumerate
    if (!cap<Cap_pcie>()) [[unlikely]]
        return;

    // Because a malicious device could implement a circular capability list, the worst-case number of ECAPs serves as a limit
    unsigned lim { (BIT (B) - O) >> A };

    // Because capabilities are 32-bit aligned, bits[1:0] of the 12-bit pointer must be masked and the result must be a valid offset or 0
    for (uint16_t ptr { O }; lim-- && (ptr &= BIT_RANGE (B - 1, A)) >= O; ) {

        auto const cap { read (Cfg::Reg32 { ptr }) };

        // Bail out for (emulated) devices that implement ECAPs incorrectly
        if (cap == BIT_RANGE (31, 0)) [[unlikely]]
            return;

        // Store capability offsets
        switch (static_cast<Ecap::Type>(cap)) {
            case Ecap::Type::SRIOV: static_cast<Cap_sriov *>(this)->off = ptr; break;
            case Ecap::Type::ATS:   static_cast<Cap_ats   *>(this)->off = ptr; break;
            case Ecap::Type::PRI:   static_cast<Cap_pri   *>(this)->off = ptr; break;
            default: break;
        }

        ptr = static_cast<uint16_t>(cap >> 20);
    }
}

bool Pci::init_bus (uint16_t const seg, uint8_t const bus, uint8_t const ebn, uint8_t const lev, Bus_bitmap &bitmap)
{
    // This bus was already enumerated
    if (bitmap.tas (bus)) [[unlikely]]
        return false;

    for (unsigned i { 0 }; i < 256; i++) {

        auto const sbdf { static_cast<pci_t>(seg << seg_shft | bus << bus_shft | i) };

        // Skip non-existing or invalid devices
        auto const didvid { *std::start_lifetime_as<uint32_t volatile> (ecam_addr (sbdf)) };
        if (didvid == 0xffffffff || didvid == 0)
            continue;

        auto const pci { new Function { sbdf, lev } };
        if (!pci) [[unlikely]]
            panic ("PCI allocation failed");

        auto const hdr { pci->read (Cfg::Reg8::HDR) };

        // PCI-PCI Bridge
        if ((hdr & BIT_RANGE (6, 0)) == 1) {
            auto const num { pci->read (Cfg::Reg32::BUS_NUM) };
            auto const sec { static_cast<uint8_t>(num >>  8) };
            auto const sub { static_cast<uint8_t>(num >> 16) };
            auto const lim { min (sub, ebn) };
            if (sec <= bus || sec > lim || !init_bus (seg, sec, lim, lev + 1, bitmap)) [[unlikely]]
                trace (TRACE_ERROR, "PCIE: %04x:%02x:%02x.%x reports invalid secondary bus %#04x", seg, Pci::bus (sbdf), Pci::dev (sbdf), Pci::fun (sbdf), sec);
        }

        // Multi-Function Device
        if (!fun (sbdf) && !(hdr & BIT (7)))
            i += 7;
    }

    return true;
}

bool Pci::init_seg (uint64_t phys, uint16_t const seg, uint8_t const sbn, uint8_t const ebn)
{
    if (seg >= seg_grps) [[unlikely]]
        return false;

    auto virt { ecam_addr (seg << seg_shft) };
    auto size { (ebn + 1) * (cfg_size << bus_shft) };

    // PCIE Specification: Segment groups must be at least 2MiB aligned
    if ((virt | phys) & Hpt::offs_mask (Hpt::bpl))
        return false;

    trace (TRACE_PCI, "PCIE: %#010lx Seg %#06x Bus %#04x-%#04x", phys, seg, sbn, ebn);

    for (unsigned o; size; size -= BITN (o), phys += BITN (o), virt += BITN (o))
        Hptp::master_map (virt, phys, (o = aligned_order (size, phys, virt)) - PAGE_BITS, Paging::Permissions (Paging::G | Paging::W | Paging::R), Memattr::dev());

    // Iterate over bus range [sbn, ebn], ignoring already enumerated buses behind bridges
    Bus_bitmap bitmap;
    for (unsigned bus { sbn }; bus <= ebn; bus++)
        init_bus (seg, static_cast<uint8_t>(bus), ebn, 0, bitmap);

    return true;
}
