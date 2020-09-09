/*
 * Unified Extensible Firmware Interface (UEFI)
 *
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

#include "std.hpp"
#include "uefi.hpp"

bool Uefi::Graphics_output_protocol::select_mode (Info &i) const
{
    uint32_t y { 0 }, m { 0 };

    if (!mode_ptr) [[unlikely]]
        return false;

    auto const mode { std::start_lifetime_as<Mode const> (mode_ptr) };

    // Iterate over all modes
    for (unsigned n { 0 }; n < mode->max; n++) {

        uintptr_t s, info_ptr;

        // Skip if unsupported mode
        if (query_mode (this, n, &s, &info_ptr) != Status::SUCCESS || !info_ptr) [[unlikely]]
            continue;

        auto const mode_info { std::start_lifetime_as<Mode_info const> (info_ptr) };

        // Skip if unsupported pixel format
        if (mode_info->pix_fmt >= Graphics_output_protocol::Pixel_format::MSK) [[unlikely]]
            continue;

        // Prefer mode with more rows
        if (y < mode_info->res_y) [[unlikely]] {
            y = mode_info->res_y;
            m = n;
        }
    }

    // Fail if mode could not be set
    if (!y || set_mode (this, m) != Status::SUCCESS) [[unlikely]]
        return false;

    // Fail if mode has no framebuffer or no info
    if (!mode->fbuf_size || !mode->info_ptr) [[unlikely]]
        return false;

    auto const mode_info { std::start_lifetime_as<Mode_info const> (mode->info_ptr) };

    // Store graphics mode information
    i.gfx.addr  = mode->fbuf_addr;
    i.gfx.size  = mode->fbuf_size;
    i.gfx.pitch = mode_info->pitch;
    i.gfx.res_x = mode_info->res_x;
    i.gfx.res_y = mode_info->res_y;

    switch (mode_info->pix_fmt) {
        case Pixel_format::RGB: i.gfx.pixel = 0xef07a0e0; break;
        case Pixel_format::BGR: i.gfx.pixel = 0xce07a2f0; break;
        default: break;
    }

    return true;
}

bool Uefi::Pci_io_protocol::should_disconnect() const
{
    uintptr_t seg, bus, dev, fun;

    // Don't disconnect devices on bus 0
    if (get_location (this, &seg, &bus, &dev, &fun) != Status::SUCCESS || bus == 0) [[unlikely]]
        return false;

    // Don't disconnect VGA controllers that may provide the GFX framebuffer
    uint16_t cls;
    if (pci_read (this, Width::UINT16, 0xa, 1, &cls) != Status::SUCCESS || cls == 0x0300) [[unlikely]]
        return false;

    return true;
}

void Uefi::Pci_io_protocol::disable_busmaster() const
{
    uintptr_t seg, bus, dev, fun;
    if (get_location (this, &seg, &bus, &dev, &fun) != Status::SUCCESS)
        return;

    uint16_t cls;
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

bool Uefi::Bsv_table::handle_gfx (handle img, Info &i) const
{
    uintptr_t cnt;
    handle *hdl;
    void *dev;

    constexpr auto uuid { Graphics_output_protocol::uuid };

    // Allocate handle buffer
    if (locate_handle_buffer (Search_type::BY_PROTOCOL, &uuid, nullptr, &cnt, &hdl) != Status::SUCCESS) [[unlikely]]
        return false;

    // Iterate over all GFX handles
    for (unsigned n { 0 }; n < cnt; n++)
        if (open_protocol (hdl[n], &uuid, &dev, img, nullptr, Open_attr::GET_PROTOCOL) == Status::SUCCESS && dev) [[likely]]
            if (std::start_lifetime_as<Graphics_output_protocol const> (dev)->select_mode (i)) [[likely]]
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
        if (open_protocol (hdl[i], &uuid, &dev, img, nullptr, Open_attr::GET_PROTOCOL) == Status::SUCCESS && dev) [[likely]]
            if (std::start_lifetime_as<Pci_io_protocol const> (dev)->should_disconnect()) [[unlikely]]
                disconnect_controller (hdl[i], nullptr, nullptr);

    // Iterate over all PCI handles and disable busmasters
    for (unsigned i { 0 }; i < cnt; i++)
        if (open_protocol (hdl[i], &uuid, &dev, img, nullptr, Open_attr::GET_PROTOCOL) == Status::SUCCESS && dev) [[likely]]
            std::start_lifetime_as<Pci_io_protocol const> (dev)->disable_busmaster();

    // Free handle buffer
    return free_pool (hdl) == Status::SUCCESS;
}

bool Uefi::Bsv_table::exit (handle img, Info &i) const
{
    void *p { nullptr };

    for (uintptr_t n { 3 }, m { 0 };;) {

        Status s; uintptr_t k, d; uint32_t v;

        if ((s = get_memory_map (&m, p, &k, &d, &v)) == Status::SUCCESS) {

            // Store memory map information
            i.mem.mmap = reinterpret_cast<uintptr_t>(p);
            i.mem.msiz = static_cast<uint32_t>(m);
            i.mem.dsiz = static_cast<uint16_t>(d);
            i.mem.dver = static_cast<uint16_t>(v);

            // Exit boot services
            return exit_boot_services (img, k) == Status::SUCCESS;
        }

        if (p) [[unlikely]]
            free_pool (p);

        // Allocate buffer of the required size plus 3 additional descriptors (d is still unknown at this point)
        if (!--n || s != Status::BUFFER_TOO_SMALL || allocate_pool (Memory_type::LDR_DATA, m += 3 * sizeof (Memory_desc), &p) != Status::SUCCESS) [[unlikely]]
            break;
    }

    return false;
}

/*
 * Must check img != nullptr and sys_table != 0 before invoking this function
 * to guarantee that it is only called while in 64bit mode with >= 128K stack
 */
void Uefi::init (handle img, uintptr_t sys_table, Info &i)
{
    auto const sys { std::start_lifetime_as<Sys_table const> (sys_table) };

    // Check if SYS table is valid
    if (!sys->valid()) [[unlikely]]
        return;

    // Handle BSV table
    if (sys->bsv_table) [[likely]] {

        auto const bsv { std::start_lifetime_as<Bsv_table const> (sys->bsv_table) };

        // Check if BSV table is valid
        if (!bsv->valid()) [[unlikely]]
            return;

        bsv->handle_gfx (img, i);
        bsv->handle_pci (img);

        // Exit boot services
        bsv->exit (img, i);
    }

    // Handle CFG table
    if (sys->cfg_table) [[likely]] {

        auto const cfg { std::start_lifetime_as_array<Cfg_table const> (sys->cfg_table, sys->cfg_entries) };

        // Parse entries
        for (unsigned n { 0 }; n < sys->cfg_entries; n++) {
            if (cfg[n].uuid == Uuid { 0x8868e871, 0xe4f1, 0x11d3, { 0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81 } }) // RSDP (2.0)
                i.tbl.rsdp = cfg[n].table;
            if (cfg[n].uuid == Uuid { 0x1878f400, 0xdcdb, 0x4f5e, { 0x8b, 0x2d, 0x85, 0x71, 0x4a, 0xca, 0x2c, 0x90 } }) // PPAM Manifest
                i.tbl.ppam = cfg[n].table;
            if (cfg[n].uuid == Uuid { 0xb1b621d5, 0xf19c, 0x41a5, { 0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0 } }) // DTBP
                i.tbl.fdtp = cfg[n].table;
        }
    }
}
