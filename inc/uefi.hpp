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

#pragma once

#include "compiler.hpp"
#include "guid.hpp"
#include "macros.hpp"

class Uefi
{
    friend class Multiboot;

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
            LOAD_ERROR          = 0x1 | BIT64 (63),
            INVALID_PARAMETER   = 0x2 | BIT64 (63),
            UNSUPPORTED         = 0x3 | BIT64 (63),
            BAD_BUFFER_SIZE     = 0x4 | BIT64 (63),
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

        // Boot Services Table (4.4)
        struct Bsv_table : Table_header
        {
            Status      (EFICALL *f1[2])();
            Status      (EFICALL *allocate_pages)(Allocate_type, Memory_type, uintn, phys_addr *);
            Status      (EFICALL *free_pages)(phys_addr, uintn);
            Status      (EFICALL *get_memory_map)(uintn *, Memory_desc *, uintn *, uintn *, uint32 *);
            Status      (EFICALL *allocate_pool)(Memory_type, uintn, void **);
            Status      (EFICALL *free_pool)(void *);
            Status      (EFICALL *f2[19])();
            Status      (EFICALL *exit_boot_services)(handle, uintn);
            Status      (EFICALL *f3[4])();
            Status      (EFICALL *disconnect_controller)(handle, handle, handle);
            Status      (EFICALL *open_protocol)(handle, Guid const *, void **, handle, handle, uint32);
            Status      (EFICALL *close_protocol)(handle, Guid const *, handle, handle);
            Status      (EFICALL *f4[2])();
            Status      (EFICALL *locate_handle_buffer)(Search_type, Guid const *, void *, uintn *, handle **);
        };

        // System Table (4.3)
        struct Sys_table : Table_header
        {
            void *      firmware_vendor;
            uint32      firmware_revision;
            handle      con_in_handle;
            void *      con_in_proto;
            handle      con_out_handle;
            void *      con_out_proto;
            handle      con_err_handle;
            void *      con_err_proto;
            void *      rsv_table;
            Bsv_table * bsv_table;
            uintn       cfg_entries;
            Cfg_table * cfg_table;

            bool valid() const
            {
                return signature == 0x5453595320494249 && size == sizeof (*this);
            }
        };

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
                Status  (EFICALL *pci_read)(Pci_io_protocol *, Width, uint32, uintn, void *);
                Status  (EFICALL *pci_write)(Pci_io_protocol *, Width, uint32, uintn, void *);
                Status  (EFICALL *f2[6])();
                Status  (EFICALL *get_location)(Pci_io_protocol *, uintn *, uintn *, uintn *, uintn *);
                Status  (EFICALL *f3[3])();
                uint64  rom_size;
                void *  rom_image;

            public:
                inline void busmaster_disable();

                static constexpr Guid guid { 0x4cf5b200, 0x68b8, 0x4ca5, 0x9a02503f3eb2ec9e };
        };

        static handle       img;
        static Sys_table *  sys;
        static Bsv_table *  bsv;

        static inline bool exit_boot_dev();
        static inline bool exit_boot_svc();

    public:
        static void *       map;
        static uintn        msz;
        static uintn        dsc;
        static uint32       ver;

        static bool init();
};
