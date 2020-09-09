/*
 * Unified Extensible Firmware Interface (UEFI)
 *
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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

#include "compiler.hpp"
#include "guid.hpp"
#include "macros.hpp"

class Uefi
{
    private:
        // Data Types (2.3.1)
        typedef void *      handle;
        typedef uintptr_t   uintn;
        typedef uint64      phys_addr;
        typedef uint64      virt_addr;

        // Status Codes (Appendix D)
        enum class Status : uintn
        {
            SUCCESS             = 0x0,
            BUFFER_TOO_SMALL    = 0x5 | BIT64 (63),
        };

        // Allocate Type (7.2)
        enum class Allocate_type
        {
            ANY_PAGES,
            MAX_ADDRESS,
            ADDRESS,
        };

        // Search Type (7.3)
        enum class Search_type
        {
            ALL_HANDLES,
            BY_REGISTER_NOTIFY,
            BY_PROTOCOL,
        };

        // Memory Type (7.2)
        enum class Memory_type
        {
            RESERVED,
            LDR_CODE,
            LDR_DATA,
            BSV_CODE,
            BSV_DATA,
            RSV_CODE,
            RSV_DATA,
            CONVENTIONAL,
            UNUSABLE,
            ACPI_RECLAIM,
            ACPI_NVS,
            MMIO,
            MMIO_PORT,
            PAL_CODE,
        };

        // Memory Descriptor (7.2)
        struct Memory_desc
        {
            uint32      type;
            phys_addr   phys;
            virt_addr   virt;
            uint64      pcnt;
            uint64      attr;
        };

        // Table Header (4.2)
        struct Table_header
        {
            uint64      signature;
            uint32      revision;
            uint32      size;
            uint32      crc;
            uint32      reserved;
        };

        // Configuration Table (4.6)
        struct Cfg_table
        {
            Guid        guid;
            void *      table;
        };

        static_assert (__is_standard_layout (Cfg_table));

        // Boot Services Table (4.4)
        struct Bsv_table
        {
            Table_header    header;
            Status (EFICALL *f1[2])();
            Status (EFICALL *allocate_pages)(Allocate_type, Memory_type, uintn, phys_addr *);
            Status (EFICALL *free_pages)(phys_addr, uintn);
            Status (EFICALL *get_memory_map)(uintn *, Memory_desc *, uintn *, uintn *, uint32 *);
            Status (EFICALL *allocate_pool)(Memory_type, uintn, void **);
            Status (EFICALL *free_pool)(void *);
            Status (EFICALL *f2[19])();
            Status (EFICALL *exit_boot_services)(handle, uintn);
            Status (EFICALL *f3[4])();
            Status (EFICALL *disconnect_controller)(handle, handle, handle);
            Status (EFICALL *open_protocol)(handle, Guid const *, void **, handle, handle, uint32);
            Status (EFICALL *close_protocol)(handle, Guid const *, handle, handle);
            Status (EFICALL *f4[2])();
            Status (EFICALL *locate_handle_buffer)(Search_type, Guid const *, void *, uintn *, handle **);
            Status (EFICALL *f5[7])();

            ALWAYS_INLINE inline bool valid() const { return header.signature == 0x56524553544f4f42 && header.size == sizeof (*this); }

            INIT bool exit_boot_dev (handle) const;
            INIT bool exit_boot_svc (handle, Uefi *) const;
        };

        static_assert (__is_standard_layout (Bsv_table));

        // System Table (4.3)
        struct Sys_table
        {
            Table_header    header;
            void *          firmware_vendor;
            uint32          firmware_revision;
            handle          con_in_handle;
            void *          con_in;
            handle          con_out_handle;
            void *          con_out;
            handle          con_err_handle;
            void *          con_err;
            void *          rsv_table;
            Bsv_table *     bsv_table;
            uintn           cfg_entries;
            Cfg_table *     cfg_table;

            ALWAYS_INLINE inline bool valid() const { return header.signature == 0x5453595320494249 && header.size == sizeof (*this); }
        };

        static_assert (__is_standard_layout (Sys_table));

        // PCI I/O Protocol (14.4)
        class Pci_io_protocol
        {
            private:
                enum class Width
                {
                    UINT8,
                    UINT16,
                    UINT32,
                    UINT64,
                };

                Status  (EFICALL *f1[6])();
                Status  (EFICALL *pci_read)(Pci_io_protocol const *, Width, uint32, uintn, void *);
                Status  (EFICALL *pci_write)(Pci_io_protocol const *, Width, uint32, uintn, void *);
                Status  (EFICALL *f2[6])();
                Status  (EFICALL *get_location)(Pci_io_protocol const *, uintn *, uintn *, uintn *, uintn *);
                Status  (EFICALL *f3[3])();
                uint64  rom_size;
                void *  rom_image;

            public:
                INIT bool should_disconnect() const;
                INIT void disable_busmaster() const;
        };

        static_assert (__is_standard_layout (Pci_io_protocol));

    public:
        uint64 rsdp { 0 };
        uint64 fdtb { 0 };
        uint64 mmap { 0 };
        uint32 msiz { 0 };
        uint16 dsiz { 0 };
        uint16 dver { 0 };

        INIT_DATA static Uefi uefi asm ("__uefi_info");

        INIT static bool init (handle, Sys_table *, Uefi *) asm ("__uefi_init");
};
