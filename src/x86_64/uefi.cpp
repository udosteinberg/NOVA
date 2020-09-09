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

#include "uefi.hpp"
#include "util.hpp"

bool Uefi::Graphics_output_protocol::select_mode (Bsv_table const *bsv, Info &i) const
{
    uint32_t y { 0 }, p { 0 }, m { 0 };

    if (!mode_ptr) [[unlikely]]
        return false;

    auto const mode { std::start_lifetime_as<Mode const> (mode_ptr) };

    // Iterate over at most 1024 modes
    for (auto n { min (mode->max, BIT (10)) }; n--;) {

        uintptr_t size; void *info_ptr;

        // Skip if mode query fails
        if (query_mode (this, n, &size, &info_ptr) != Status::SUCCESS || !info_ptr) [[unlikely]]
            continue;

        if (size >= sizeof (Mode_info)) [[likely]] {

            auto const mode_info { std::start_lifetime_as<Mode_info const> (info_ptr) };

            // Prefer mode with more rows (or same rows and more cols)
            if (mode_info->pix_fmt < Graphics_output_protocol::Pixel_format::MSK && (y < mode_info->res_y || (y == mode_info->res_y && p < mode_info->pitch))) [[unlikely]] {
                y = mode_info->res_y;
                p = mode_info->pitch;
                m = n;
            }
        }

        // Free firmware-allocated buffer
        bsv->free_pool (info_ptr);
    }

    // Fail if geometry is invalid or if the mode could not be set
    if (!y || !p || set_mode (this, m) != Status::SUCCESS) [[unlikely]]
        return false;

    // Fail if mode info is invalid or no framebuffer exists
    if (mode->info_size < sizeof (Mode_info) || !mode->info_ptr || !mode->fbuf_size) [[unlikely]]
        return false;

    auto const mode_info { std::start_lifetime_as<Mode_info const> (mode->info_ptr) };

    // Fail if the set mode is inconsistent with the validated mode
    if (mode_info->pitch != p || mode_info->res_x > p || mode_info->res_y != y) [[unlikely]]
        return false;

    // Fail if the pixel format is unsupported
    switch (mode_info->pix_fmt) {
        default: return false;
        case Pixel_format::RGB: i.gfx.pixel = 0xef07a0e0; break;
        case Pixel_format::BGR: i.gfx.pixel = 0xce07a2f0; break;
    }

    // Store graphics mode information
    i.gfx.addr  = mode->fbuf_addr;
    i.gfx.size  = mode->fbuf_size;
    i.gfx.pitch = mode_info->pitch;
    i.gfx.res_x = mode_info->res_x;
    i.gfx.res_y = mode_info->res_y;

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

    // Iterate over at most 65536 GFX handles
    for (auto n { min (cnt, BIT (16)) }; n--;)
        if (open_protocol (hdl[n], &uuid, &dev, img, nullptr, Open_attr::GET_PROTOCOL) == Status::SUCCESS && dev) [[likely]]
            if (std::start_lifetime_as<Graphics_output_protocol const> (dev)->select_mode (this, i)) [[likely]]
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

    // Iterate over at most 65536 PCI handles and disconnect drivers
    for (auto n { min (cnt, BIT (16)) }; n--;)
        if (open_protocol (hdl[n], &uuid, &dev, img, nullptr, Open_attr::GET_PROTOCOL) == Status::SUCCESS && dev) [[likely]]
            if (std::start_lifetime_as<Pci_io_protocol const> (dev)->should_disconnect()) [[unlikely]]
                disconnect_controller (hdl[n], nullptr, nullptr);

    // Iterate over at most 65536 PCI handles and disable busmasters
    for (auto n { min (cnt, BIT (16)) }; n--;)
        if (open_protocol (hdl[n], &uuid, &dev, img, nullptr, Open_attr::GET_PROTOCOL) == Status::SUCCESS && dev) [[likely]]
            std::start_lifetime_as<Pci_io_protocol const> (dev)->disable_busmaster();

    // Free handle buffer
    return free_pool (hdl) == Status::SUCCESS;
}

bool Uefi::Bsv_table::exit (handle img, Info &i) const
{
    void *p { nullptr };

    for (uintptr_t n { 5 }, m { 0 };;) {

        uintptr_t k, d; uint32_t v;

        // Obtain memory map and exit boot services (both can fail on the first attempt)
        if (get_memory_map (&m, p, &k, &d, &v) == Status::SUCCESS && exit_boot_services (img, k) == Status::SUCCESS) {

            // Store memory map information
            i.mem.mmap = reinterpret_cast<uintptr_t>(p);
            i.mem.msiz = static_cast<uint32_t>(m);
            i.mem.dsiz = static_cast<uint16_t>(d);
            i.mem.dver = static_cast<uint16_t>(v);

            return true;
        }

        if (p) [[unlikely]]
            free_pool (p);

        // Allocate buffer of the required size plus headroom for 3 additional descriptors (d is still unknown at this point)
        if (!--n || allocate_pool (Memory_type::LDR_DATA, m += 3 * sizeof (Memory_desc), &p) != Status::SUCCESS) [[unlikely]]
            return false;
    }
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
    if (auto const tbl { sys->bsv_table }) [[likely]] {

        auto const bsv { std::start_lifetime_as<Bsv_table const> (tbl) };

        // Check if BSV table is valid
        if (!bsv->valid()) [[unlikely]]
            return;

        bsv->handle_gfx (img, i);
        bsv->handle_pci (img);

        // Exit boot services
        bsv->exit (img, i);
    }

    // Handle CFG table
    if (auto const tbl { sys->cfg_table }) [[likely]] {

        // Iterate over at most 1024 entries
        auto num { min (sys->cfg_entries, BIT (10)) };
        auto cfg { std::start_lifetime_as_array<Cfg_table const> (tbl, num) };

        for (; num--; cfg++) {
            if (cfg->uuid == Uuid { 0x8868e871, 0xe4f1, 0x11d3, { 0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81 } }) // RSDP (2.0)
                i.tbl.rsdp = cfg->table;
            if (cfg->uuid == Uuid { 0x1878f400, 0xdcdb, 0x4f5e, { 0x8b, 0x2d, 0x85, 0x71, 0x4a, 0xca, 0x2c, 0x90 } }) // PPAM Manifest
                i.tbl.ppam = cfg->table;
            if (cfg->uuid == Uuid { 0xb1b621d5, 0xf19c, 0x41a5, { 0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0 } }) // DTBP
                i.tbl.fdtp = cfg->table;
        }
    }
}
