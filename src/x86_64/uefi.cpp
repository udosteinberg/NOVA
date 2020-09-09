/*
 * Unified Extensible Firmware Interface (UEFI)
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

#include "uefi.hpp"

bool Uefi::Pci_io_protocol::should_disconnect() const
{
    uintn seg, bus, dev, fun;
    if (get_location (this, &seg, &bus, &dev, &fun) != Status::SUCCESS)
        return false;

    return bus;
}

void Uefi::Pci_io_protocol::disable_busmaster() const
{
    uintn seg, bus, dev, fun;
    if (get_location (this, &seg, &bus, &dev, &fun) != Status::SUCCESS)
        return;

    uint32_t cls;
    if (pci_read (this, Width::UINT16, 0xa, 1, &cls) != Status::SUCCESS || cls != 0x0604)
        return;

    uint16_t cmd;
    if (pci_read (this, Width::UINT16, 0x4, 1, &cmd) != Status::SUCCESS)
        return;

    if (cmd & BIT (2)) {
        cmd &= static_cast<uint16_t>(~BIT (2));
        pci_write (this, Width::UINT16, 0x4, 1, &cmd);
    }
}

bool Uefi::Bsv_table::exit_boot_dev (handle img) const
{
    uintn   cnt;
    handle *hdl;
    void   *dev;

    constexpr Uuid pci { 0x4cf5b200, 0x68b8, 0x4ca5, { 0x9e, 0xec, 0xb2, 0x3e, 0x3f, 0x50, 0x02, 0x9a } };

    // Obtain PCI handle buffer
    if (locate_handle_buffer (Search_type::BY_PROTOCOL, &pci, nullptr, &cnt, &hdl) != Status::SUCCESS)
        return false;

    // Iterate over all PCI handles and disconnect drivers
    for (unsigned i { 0 }; i < cnt; i++)
        if (open_protocol (hdl[i], &pci, &dev, img, nullptr, BIT (1)) == Status::SUCCESS)
            if (static_cast<Pci_io_protocol *>(dev)->should_disconnect())
                disconnect_controller (hdl[i], nullptr, nullptr);

    // Iterate over all PCI handles and disable busmasters
    for (unsigned i { 0 }; i < cnt; i++)
        if (open_protocol (hdl[i], &pci, &dev, img, nullptr, BIT (1)) == Status::SUCCESS)
            static_cast<Pci_io_protocol *>(dev)->disable_busmaster();

    // Free PCI handle buffer
    return free_pool (hdl) == Status::SUCCESS;
}

bool Uefi::Bsv_table::exit_boot_svc (handle img, Info *info) const
{
    void *p = nullptr;
    uintn d, k, m;
    uint32_t v;

    for (auto i = m = 0; i < 3; i++) {

        Status s;

        if ((s = get_memory_map (&m, static_cast<Memory_desc *>(p), &k, &d, &v)) == Status::SUCCESS)
            break;

        if (p)
            free_pool (p);

        p = nullptr;

        if (s != Status::BUFFER_TOO_SMALL)
            return false;

        if (allocate_pool (Memory_type::LDR_DATA, m += 2 * d, &p) != Status::SUCCESS)
            return false;
    }

    info->mmap = reinterpret_cast<uintptr_t>(p);
    info->msiz = static_cast<uint32_t>(m);
    info->dsiz = static_cast<uint16_t>(d);
    info->dver = static_cast<uint16_t>(v);

    return exit_boot_services (img, k) == Status::SUCCESS;
}

void Uefi::init (handle img, Sys_table *sys, Info *info)
{
    // UEFI unavailable
    if (!sys->valid())
        return;

    auto const bsv { sys->bsv_table };
    auto const cfg { sys->cfg_table };

    // Boot services unavailable
    if (!bsv || !bsv->valid())
        return;

    // Shut down boot services
    bsv->exit_boot_dev (img);
    bsv->exit_boot_svc (img, info);

    // Parse configuration table
    for (unsigned i { 0 }; i < sys->cfg_entries; i++) {
        if (cfg[i].uuid == Uuid { 0xeb9d2d30, 0x2d88, 0x11d3, { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d }})  // RSDP (1.0)
            info->rsdp = reinterpret_cast<uintptr_t>(cfg[i].table);
        if (cfg[i].uuid == Uuid { 0x8868e871, 0xe4f1, 0x11d3, { 0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81 }})  // RSDP (2.0)
            info->rsdp = reinterpret_cast<uintptr_t>(cfg[i].table);
        if (cfg[i].uuid == Uuid { 0x1878f400, 0xdcdb, 0x4f5e, { 0x8b, 0x2d, 0x85, 0x71, 0x4a, 0xca, 0x2c, 0x90 }})  // PPAM Manifest
            info->ppam = reinterpret_cast<uintptr_t>(cfg[i].table);
        if (cfg[i].uuid == Uuid { 0xb1b621d5, 0xf19c, 0x41a5, { 0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0 }})  // DTBP
            info->fdtp = reinterpret_cast<uintptr_t>(cfg[i].table);
    }
}
