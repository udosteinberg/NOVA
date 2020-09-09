/*
 * Unified Extensible Firmware Interface (UEFI)
 *
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

#pragma once

#include "compiler.hpp"
#include "macros.hpp"
#include "signature.hpp"
#include "uuid.hpp"

class Uefi final
{
    public:
        struct Info
        {
            uint64_t    rsdp;
            uint64_t    fdtp;
            uint64_t    mmap;
            uint32_t    msiz;
            uint16_t    dsiz;
            uint16_t    dver;
        };

    private:
        // Data Types (2.3.1)
        using handle    = void *;
        using uintn     = uintptr_t;
        using phys_addr = uint64_t;
        using virt_addr = uint64_t;

        // Status Codes (Appendix D)
        enum class Status : uintn
        {
            SUCCESS             = 0x0,
            BUFFER_TOO_SMALL    = 0x5 | BIT64 (63),
        };

        // Allocate Type (7.2)
        enum class Allocate_type : unsigned
        {
            ANY_PAGES,
            MAX_ADDRESS,
            ADDRESS,
        };

        // Search Type (7.3)
        enum class Search_type : unsigned
        {
            ALL_HANDLES,
            BY_REGISTER_NOTIFY,
            BY_PROTOCOL,
        };

        // Memory Type (7.2)
        enum class Memory_type : unsigned
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
            uint32_t    type;
            phys_addr   phys;
            virt_addr   virt;
            uint64_t    pcnt;
            uint64_t    attr;
        };

        // Table Header (4.2)
        struct Table_header
        {
            uint64_t    signature;
            uint32_t    revision;
            uint32_t    size;
            uint32_t    crc;
            uint32_t    reserved;
        };

        // Configuration Table (4.6)
        struct Cfg_table
        {
            Uuid        uuid;
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
            Status (EFICALL *get_memory_map)(uintn *, Memory_desc *, uintn *, uintn *, uint32_t *);
            Status (EFICALL *allocate_pool)(Memory_type, uintn, void **);
            Status (EFICALL *free_pool)(void *);
            Status (EFICALL *f2[19])();
            Status (EFICALL *exit_boot_services)(handle, uintn);
            Status (EFICALL *f3[4])();
            Status (EFICALL *disconnect_controller)(handle, handle, handle);
            Status (EFICALL *open_protocol)(handle, Uuid const *, void **, handle, handle, uint32_t);
            Status (EFICALL *close_protocol)(handle, Uuid const *, handle, handle);
            Status (EFICALL *f4[2])();
            Status (EFICALL *locate_handle_buffer)(Search_type, Uuid const *, void *, uintn *, handle **);
            Status (EFICALL *f5[7])();

            [[nodiscard]] bool valid() const { return header.signature == Signature::value ("BOOTSERV") && header.size == sizeof (*this); }

            SEC_INIT bool exit_boot_dev (handle) const;
            SEC_INIT bool exit_boot_svc (handle, Info *) const;
        };

        static_assert (__is_standard_layout (Bsv_table));

        // System Table (4.3)
        struct Sys_table
        {
            Table_header    header;
            void *          firmware_vendor;
            uint32_t        firmware_revision;
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

            [[nodiscard]] bool valid() const { return header.signature == Signature::value ("IBI SYST") && header.size == sizeof (*this); }
        };

        static_assert (__is_standard_layout (Sys_table));

        // PCI I/O Protocol (14.4)
        class Pci_io_protocol final
        {
            private:
                enum class Width : unsigned
                {
                    UINT8,
                    UINT16,
                    UINT32,
                    UINT64,
                };

                Status      (EFICALL *f1[6])();
                Status      (EFICALL *pci_read)(Pci_io_protocol const *, Width, uint32_t, uintn, void *);
                Status      (EFICALL *pci_write)(Pci_io_protocol const *, Width, uint32_t, uintn, void *);
                Status      (EFICALL *f2[6])();
                Status      (EFICALL *get_location)(Pci_io_protocol const *, uintn *, uintn *, uintn *, uintn *);
                Status      (EFICALL *f3[3])();
                uint64_t    rom_size;
                void *      rom_image;

            public:
                SEC_INIT bool should_disconnect() const;
                SEC_INIT void disable_busmaster() const;
        };

        static_assert (__is_standard_layout (Pci_io_protocol));

    public:
        // UEFI parameters must be in a non-BSS section
        SEC_DATA static inline Info info asm ("uefi_info") { 0, 0, 0, 0, 0, 0 };

        SEC_INIT static void init (handle, Sys_table *, Info *) asm ("uefi_init");
};
