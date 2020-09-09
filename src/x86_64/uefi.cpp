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

bool Uefi::Graphics_output_protocol::select_mode (Info &info)
{
    uint32_t mode_res { 0 }, mode_num { 0 };

    // Iterate over all modes
    for (unsigned n { 0 }; n < mode->max; n++) {

        uintptr_t s; Graphics_output_protocol::Mode_info const *i;

        // Skip if unsupported mode
        if (query_mode (this, n, &s, &i) != Status::SUCCESS) [[unlikely]]
            continue;

        // Skip if unsupported pixel format
        if (i->pix_fmt >= Graphics_output_protocol::Pixel_format::MSK) [[unlikely]]
            continue;

        // Determine mode resolution
        auto const r { i->res_x * i->res_y };

        // Prefer mode with higher resolution
        if (mode_res < r) [[unlikely]] {
            mode_res = r;
            mode_num = n;
        }
    }

    // Fail if mode could not be set
    if (!mode_res || set_mode (this, mode_num) != Status::SUCCESS) [[unlikely]]
        return false;

    // Fail if mode has no framebuffer
    if (!mode->fbuf_size) [[unlikely]]
        return false;

    // Store graphics mode information
    info.gfx.addr  = mode->fbuf_addr;
    info.gfx.size  = mode->fbuf_size;
    info.gfx.pitch = mode->info->pitch;
    info.gfx.res_x = mode->info->res_x;
    info.gfx.res_y = mode->info->res_y;

    switch (mode->info->pix_fmt) {
        case Pixel_format::RGB: info.gfx.pixel = 0xef07a0e0; break;
        case Pixel_format::BGR: info.gfx.pixel = 0xce07a2f0; break;
        default: break;
    }

    return true;
}

bool Uefi::Pci_io_protocol::should_disconnect() const
{
    uintptr_t seg, bus, dev, fun;
    if (get_location (this, &seg, &bus, &dev, &fun) != Status::SUCCESS) [[unlikely]]
        return false;

    return bus;
}

void Uefi::Pci_io_protocol::disable_busmaster() const
{
    uintptr_t seg, bus, dev, fun;
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

bool Uefi::Bsv_table::handle_gfx (handle img, Info &info) const
{
    uintptr_t cnt;
    handle *hdl;
    void *dev;

    constexpr auto uuid { Graphics_output_protocol::uuid };

    // Allocate handle buffer
    if (locate_handle_buffer (Search_type::BY_PROTOCOL, &uuid, nullptr, &cnt, &hdl) != Status::SUCCESS) [[unlikely]]
        return false;

    // Iterate over all GFX handles
    for (unsigned i { 0 }; i < cnt; i++)
        if (open_protocol (hdl[i], &uuid, &dev, img, nullptr, Open_attr::GET_PROTOCOL) == Status::SUCCESS) [[likely]]
            if (static_cast<Graphics_output_protocol *>(dev)->select_mode (info)) [[likely]]
                break;

    // Free handle buffer
    return free_pool (hdl) == Status::SUCCESS;
}

bool Uefi::Bsv_table::handle_pci (handle img) const
{
    uintptr_t cnt;
    handle *hdl;
    void *dev;

    constexpr auto uuid { Pci_io_protocol::uuid };

    // Allocate handle buffer
    if (locate_handle_buffer (Search_type::BY_PROTOCOL, &uuid, nullptr, &cnt, &hdl) != Status::SUCCESS) [[unlikely]]
        return false;

    // Iterate over all PCI handles and disconnect drivers
    for (unsigned i { 0 }; i < cnt; i++)
        if (open_protocol (hdl[i], &uuid, &dev, img, nullptr, Open_attr::GET_PROTOCOL) == Status::SUCCESS) [[likely]]
            if (static_cast<Pci_io_protocol *>(dev)->should_disconnect()) [[unlikely]]
                disconnect_controller (hdl[i], nullptr, nullptr);

    // Iterate over all PCI handles and disable busmasters
    for (unsigned i { 0 }; i < cnt; i++)
        if (open_protocol (hdl[i], &uuid, &dev, img, nullptr, Open_attr::GET_PROTOCOL) == Status::SUCCESS) [[likely]]
            static_cast<Pci_io_protocol *>(dev)->disable_busmaster();

    // Free handle buffer
    return free_pool (hdl) == Status::SUCCESS;
}

bool Uefi::Bsv_table::exit (handle img, Info &info) const
{
    void *p { nullptr };
    uintptr_t d, k, m;
    uint32_t v;

    for (auto i = m = 0; i < 3; i++) {

        Status s;

        if ((s = get_memory_map (&m, static_cast<Memory_desc *>(p), &k, &d, &v)) == Status::SUCCESS)
            break;

        if (p)
            free_pool (p);

        if (s != Status::BUFFER_TOO_SMALL) [[unlikely]]
            return false;

        if (allocate_pool (Memory_type::LDR_DATA, m += 2 * d, &p) != Status::SUCCESS) [[unlikely]]
            return false;
    }

    // Store memory map information
    info.mem.mmap = reinterpret_cast<uintptr_t>(p);
    info.mem.msiz = static_cast<uint32_t>(m);
    info.mem.dsiz = static_cast<uint16_t>(d);
    info.mem.dver = static_cast<uint16_t>(v);

    return exit_boot_services (img, k) == Status::SUCCESS;
}

/*
 * Must check img != nullptr and sys != nullptr before invoking this function
 * to guarantee that it is only called while in 64bit mode with >= 128K stack
 */
void Uefi::init (handle img, Sys_table *sys, Info &info)
{
    // Check system table
    if (!sys->valid()) [[unlikely]]
        return;

    // Check boot services table
    auto const bsv { sys->bsv_table };
    if (!bsv || !bsv->valid()) [[unlikely]]
        return;

    bsv->handle_gfx (img, info);
    bsv->handle_pci (img);

    // Exit boot services
    bsv->exit (img, info);

    // Check configuration table
    auto const cfg { sys->cfg_table };
    if (!cfg) [[unlikely]]
        return;

    // Parse configuration table
    for (unsigned i { 0 }; i < sys->cfg_entries; i++) {
        if (cfg[i].uuid == Uuid { 0xeb9d2d30, 0x2d88, 0x11d3, { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } }) // RSDP (1.0)
            info.tbl.rsdp = cfg[i].table;
        if (cfg[i].uuid == Uuid { 0x8868e871, 0xe4f1, 0x11d3, { 0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81 } }) // RSDP (2.0)
            info.tbl.rsdp = cfg[i].table;
        if (cfg[i].uuid == Uuid { 0x1878f400, 0xdcdb, 0x4f5e, { 0x8b, 0x2d, 0x85, 0x71, 0x4a, 0xca, 0x2c, 0x90 } }) // PPAM Manifest
            info.tbl.ppam = cfg[i].table;
        if (cfg[i].uuid == Uuid { 0xb1b621d5, 0xf19c, 0x41a5, { 0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0 } }) // DTBP
            info.tbl.fdtp = cfg[i].table;
    }
}
