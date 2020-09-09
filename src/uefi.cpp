/*
 * Unified Extensible Firmware Interface (UEFI)
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

#include "uefi.hpp"

Uefi Uefi::uefi;

void Uefi::Pci_io_protocol::busmaster_disable()
{
    uintn seg, bus, dev, fun;
    if (get_location (this, &seg, &bus, &dev, &fun) != Status::SUCCESS)
        return;

    uint32 idr;
    if (pci_read (this, Width::UINT32, 0x0, 1, &idr) != Status::SUCCESS || idr == BIT_RANGE (31, 0))
        return;

    uint16 cmd;
    if (pci_read (this, Width::UINT16, 0x4, 1, &cmd) != Status::SUCCESS)
        return;

    if (cmd & BIT (2)) {
        cmd &= static_cast<uint16>(~BIT (2));
        pci_write (this, Width::UINT16, 0x4, 1, &cmd);
    }
}

bool Uefi::Bsv_table::exit_boot_dev (handle img)
{
    uintn   cnt;
    handle *hdl;
    void   *ifc;

    constexpr Guid guid { 0x4cf5b200, 0x68b8, 0x4ca5, 0x9a02503f3eb2ec9e };

    // Obtain PCI handle buffer
    if (locate_handle_buffer (Search_type::BY_PROTOCOL, &guid, nullptr, &cnt, &hdl) != Status::SUCCESS)
        return false;

    // Iterate over all PCI handles and disconnect drivers
    for (unsigned i = 0; i < cnt; i++)
        disconnect_controller (hdl[i], nullptr, nullptr);

    // Iterate over all PCI handles and disable busmasters
    for (unsigned i = 0; i < cnt; i++)
        if (open_protocol (hdl[i], &guid, &ifc, img, nullptr, BIT (1)) == Status::SUCCESS)
            static_cast<Pci_io_protocol *>(ifc)->busmaster_disable();

    // Free PCI handle buffer
    return free_pool (hdl) == Status::SUCCESS;
}

bool Uefi::Bsv_table::exit_boot_svc (handle img, Uefi *info)
{
    void *p = nullptr;
    uintn d, k, m;
    uint32 v;

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
    info->msiz = static_cast<uint32>(m);
    info->dsiz = static_cast<uint16>(d);
    info->dver = static_cast<uint16>(v);

    return exit_boot_services (img, k) == Status::SUCCESS;
}

bool Uefi::init (handle img, Sys_table *sys, Uefi *info)
{
    // UEFI unavailable
    if (!sys->valid())
        return false;

    auto bsv = sys->bsv_table;
    auto cfg = sys->cfg_table;
    auto cnt = sys->cfg_entries;

    // Parse configuration table
    for (unsigned i = 0; i < cnt; i++)
        if (cfg[i].guid == Guid { 0x8868e871, 0xe4f1, 0x11d3, 0x81883cc7800022bc })
            info->rsdp = reinterpret_cast<uintptr_t>(cfg[i].table);

    // Boot services unavailable
    if (!bsv)
        return false;

    return bsv->exit_boot_dev (img) & bsv->exit_boot_svc (img, info);
}
