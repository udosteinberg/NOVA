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

#include "acpi.hpp"
#include "cmdline.hpp"
#include "stdio.hpp"
#include "uefi.hpp"

Uefi::handle        Uefi::img { nullptr };
Uefi::Sys_table *   Uefi::sys { nullptr };
Uefi::Bsv_table *   Uefi::bsv { nullptr };
void *              Uefi::map { nullptr };
Uefi::uintn         Uefi::msz { 0 };
Uefi::uintn         Uefi::dsc { 0 };
uint32              Uefi::ver { 0 };

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

    trace (TRACE_FIRM, "UEFI: PCI %04lx:%02lx:%02lx.%lx %04x:%04x %c%c%c",
           seg, bus, dev, fun,
           idr       & BIT_RANGE (15, 0),
           idr >> 16 & BIT_RANGE (15, 0),
           cmd & BIT (2) ? 'B' : '-',
           cmd & BIT (1) ? 'M' : '-',
           cmd & BIT (0) ? 'I' : '-');

    if (cmd & BIT (2)) {
        cmd &= static_cast<uint16>(~BIT (2));
        pci_write (this, Width::UINT16, 0x4, 1, &cmd);
    }
}

bool Uefi::exit_boot_dev()
{
    uintn   cnt;
    handle *hdl;
    void   *ifc;

    if (Cmdline::noquiesce)
        return false;

    // Obtain PCI handle buffer
    if (bsv->locate_handle_buffer (Search_type::BY_PROTOCOL, &Pci_io_protocol::guid, nullptr, &cnt, &hdl) != Status::SUCCESS)
        return false;

    // Iterate over all PCI handles and disconnect drivers
    for (unsigned i = 0; i < cnt; i++)
        bsv->disconnect_controller (hdl[i], nullptr, nullptr);

    // Iterate over all PCI handles and disable busmasters
    for (unsigned i = 0; i < cnt; i++)
        if (bsv->open_protocol (hdl[i], &Pci_io_protocol::guid, &ifc, img, nullptr, BIT (1)) == Status::SUCCESS)
            static_cast<Pci_io_protocol *>(ifc)->busmaster_disable();

    // Free PCI handle buffer
    return bsv->free_pool (hdl) == Status::SUCCESS;
}

bool Uefi::exit_boot_svc()
{
    uintn key;
    Status sts;

    for (unsigned i = 0; i < 3; i++) {

        if ((sts = bsv->get_memory_map (&msz, static_cast<Memory_desc *>(map), &key, &dsc, &ver)) == Status::SUCCESS)
            break;

        if (map)
            bsv->free_pool (map);

        map = nullptr;

        if (sts != Status::BUFFER_TOO_SMALL)
            return false;

        if (bsv->allocate_pool (Memory_type::LDR_DATA, msz += 2 * dsc, &map) != Status::SUCCESS)
            return false;
    }

    return bsv->exit_boot_services (img, key) == Status::SUCCESS;
}

bool Uefi::init()
{
    // UEFI unavailable
    if (!sys || !sys->valid())
        return false;

    trace (TRACE_FIRM, "UEFI: Version %u.%u.%u", sys->revision >> 16, (sys->revision & 0xffff) / 10, (sys->revision & 0xffff) % 10);

    auto cfg = sys->cfg_table;
    auto cnt = sys->cfg_entries;

    // Parse configuration table
    for (unsigned i = 0; i < cnt; i++)
        if (cfg[i].guid == Guid { 0x8868e871, 0xe4f1, 0x11d3, 0x81883cc7800022bc })
            Acpi::rsdp = Paddr (cfg[i].table);

    // Boot services or image handle unavailable
    if (!(bsv = sys->bsv_table) || !img)
        return false;

    return exit_boot_dev() & exit_boot_svc();
}
