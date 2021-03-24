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
            struct {
                uint64_t    rsdp;
                uint64_t    ppam;
                uint64_t    fdtp;
            } tbl;

            struct {
                uint64_t    mmap;
                uint32_t    msiz;
                uint16_t    dsiz;
                uint16_t    dver;
            } mem;

            struct {
                uint64_t    addr;
                uint64_t    size;
                uint32_t    pixel;
                uint32_t    pitch;
                uint32_t    res_x;
                uint32_t    res_y;

                auto b() const { return BIT_RANGE (pixel >> 25 & BIT_RANGE (4, 0), pixel >> 20 & BIT_RANGE (4, 0)); }
                auto g() const { return BIT_RANGE (pixel >> 15 & BIT_RANGE (4, 0), pixel >> 10 & BIT_RANGE (4, 0)); }
                auto r() const { return BIT_RANGE (pixel >>  5 & BIT_RANGE (4, 0), pixel >>  0 & BIT_RANGE (4, 0)); }
            } gfx;
        };

    private:
        /*
         * Data Types (2.3.1)
         */
        using handle    = void *;
        using phys_addr = uint64_t;
        using virt_addr = uint64_t;

        /*
         * Status Codes (Appendix D)
         */
        enum class Status : uintptr_t
        {
            SUCCESS             = 0x0,
            BUFFER_TOO_SMALL    = 0x5 | BIT64 (63),
        };

        /*
         * Allocate Type (7.2)
         */
        enum class Allocate_type : uint32_t
        {
            ANY_PAGES,
            MAX_ADDRESS,
            ADDRESS,
        };

        /*
         * Search Type (7.3)
         */
        enum class Search_type : uint32_t
        {
            ALL_HANDLES,
            BY_REGISTER_NOTIFY,
            BY_PROTOCOL,
        };

        /*
         * Protocol Open Attributes (7.3)
         */
        enum class Open_attr : uint32_t
        {
            BY_HANDLE_PROTOCOL  = BIT (0),
            GET_PROTOCOL        = BIT (1),
            TEST_PROTOCOL       = BIT (2),
            BY_CHILD_CONTROLLER = BIT (3),
            BY_DRIVER           = BIT (4),
            EXCLUSIVE           = BIT (5),
        };

        /*
         * Memory Type (7.2)
         */
        enum class Memory_type : uint32_t
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
            PERSISTENT,
            UNACCEPTED,
        };

        /*
         * Memory Descriptor (7.2)
         */
        struct Memory_desc final
        {
            uint32_t    type;
            phys_addr   phys;
            virt_addr   virt;
            uint64_t    pcnt;
            uint64_t    attr;
        };

        static_assert (__is_standard_layout (Memory_desc) && alignof (Memory_desc) == 8 && sizeof (Memory_desc) == 40);

        /*
         * Table Header (4.2)
         */
        struct Table_header final
        {
            uint64_t    signature;
            uint32_t    revision;
            uint32_t    size;
            uint32_t    crc;
            uint32_t    reserved;
        };

        static_assert (__is_standard_layout (Table_header) && alignof (Table_header) == 8 && sizeof (Table_header) == 24);

        /*
         * Configuration Table (4.6)
         */
        struct Cfg_table final
        {
            Uuid        uuid;
            uintptr_t   table;
        };

        static_assert (__is_standard_layout (Cfg_table) && alignof (Cfg_table) == 8 && sizeof (Cfg_table) == 24);

        /*
         * Boot Services Table (4.4)
         */
        struct Bsv_table final
        {
            Table_header    header;
            Status (EFICALL *f1[2])();
            Status (EFICALL *allocate_pages)(Allocate_type, Memory_type, uintptr_t, phys_addr *);                   // 7.2.1
            Status (EFICALL *free_pages)(phys_addr, uintptr_t);                                                     // 7.2.2
            Status (EFICALL *get_memory_map)(uintptr_t *, void *, uintptr_t *, uintptr_t *, uint32_t *);            // 7.2.3
            Status (EFICALL *allocate_pool)(Memory_type, uintptr_t, void **);                                       // 7.2.4
            Status (EFICALL *free_pool)(void *);                                                                    // 7.2.5
            Status (EFICALL *f2[19])();
            Status (EFICALL *exit_boot_services)(handle, uintptr_t);                                                // 7.4.6
            Status (EFICALL *f3[4])();
            Status (EFICALL *disconnect_controller)(handle, handle, handle);                                        // 7.3.13
            Status (EFICALL *open_protocol)(handle, Uuid const *, void **, handle, handle, Open_attr);              // 7.3.9
            Status (EFICALL *close_protocol)(handle, Uuid const *, handle, handle);                                 // 7.3.10
            Status (EFICALL *f4[2])();
            Status (EFICALL *locate_handle_buffer)(Search_type, Uuid const *, void *, uintptr_t *, handle **);      // 7.3.15
            Status (EFICALL *f5[7])();

            [[nodiscard]] SEC_INIT bool valid() const { return header.signature == Signature::u64 ("BOOTSERV") && header.size == sizeof (*this); }

            SEC_INIT bool handle_gfx (handle, Info &) const;
            SEC_INIT bool handle_pci (handle) const;
            SEC_INIT bool exit (handle, Info &) const;
        };

        static_assert (__is_standard_layout (Bsv_table) && alignof (Bsv_table) == 8 && sizeof (Bsv_table) == 376);

        /*
         * System Table (4.3)
         */
        struct Sys_table final
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
            uintptr_t       bsv_table;
            uintptr_t       cfg_entries;
            uintptr_t       cfg_table;

            [[nodiscard]] SEC_INIT bool valid() const { return header.signature == Signature::u64 ("IBI SYST") && header.size == sizeof (*this); }
        };

        static_assert (__is_standard_layout (Sys_table) && alignof (Sys_table) == 8 && sizeof (Sys_table) == 120);

        /*
         * Graphics Output Protocol (12.9.2)
         */
        class Graphics_output_protocol final
        {
            public:
                static constexpr Uuid uuid { 0x9042a9de, 0x23dc, 0x4a38, { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } };

                SEC_INIT bool select_mode (Info &) const;

            private:
                enum class Pixel_format : uint32_t
                {
                    RGB = 0,
                    BGR = 1,
                    MSK = 2,
                    BLT = 3,
                };

                struct Pixel_bitmask
                {
                    uint32_t        r, g, b, u;
                };

                static_assert (__is_standard_layout (Pixel_bitmask) && alignof (Pixel_bitmask) == 4 && sizeof (Pixel_bitmask) == 16);

                struct Mode_info
                {
                    uint32_t        version;
                    uint32_t        res_x;
                    uint32_t        res_y;
                    Pixel_format    pix_fmt;
                    Pixel_bitmask   pix_msk;
                    uint32_t        pitch;
                };

                static_assert (__is_standard_layout (Mode_info) && alignof (Mode_info) == 4 && sizeof (Mode_info) == 36);

                struct Mode
                {
                    uint32_t        max;
                    uint32_t        cur;
                    uintptr_t       info_ptr;
                    uintptr_t       info_size;
                    phys_addr       fbuf_addr;
                    uintptr_t       fbuf_size;
                };

                static_assert (__is_standard_layout (Mode) && alignof (Mode) == 8 && sizeof (Mode) == 40);

                Status (EFICALL *query_mode)(Graphics_output_protocol const *, uint32_t, uintptr_t *, uintptr_t *);             // 12.9.2.1
                Status (EFICALL *set_mode)  (Graphics_output_protocol const *, uint32_t);                                       // 12.9.2.2
                Status (EFICALL *f1[1])     (Graphics_output_protocol const *);
                uintptr_t mode_ptr;
        };

        static_assert (__is_standard_layout (Graphics_output_protocol) && alignof (Graphics_output_protocol) == 8 && sizeof (Graphics_output_protocol) == 32);

        /*
         * PCI I/O Protocol (14.4)
         */
        class Pci_io_protocol final
        {
            public:
                static constexpr Uuid uuid { 0x4cf5b200, 0x68b8, 0x4ca5, { 0x9e, 0xec, 0xb2, 0x3e, 0x3f, 0x50, 0x02, 0x9a } };

                SEC_INIT bool should_disconnect() const;
                SEC_INIT void disable_busmaster() const;

            private:
                enum class Width : uint32_t
                {
                    UINT8,
                    UINT16,
                    UINT32,
                    UINT64,
                };

                Status (EFICALL *f1[6])       (Pci_io_protocol const *);
                Status (EFICALL *pci_read)    (Pci_io_protocol const *, Width, uint32_t, uintptr_t, void *);                    // 14.4.8
                Status (EFICALL *pci_write)   (Pci_io_protocol const *, Width, uint32_t, uintptr_t, void *);                    // 14.4.9
                Status (EFICALL *f2[6])       (Pci_io_protocol const *);
                Status (EFICALL *get_location)(Pci_io_protocol const *, uintptr_t *, uintptr_t *, uintptr_t *, uintptr_t *);    // 14.4.16
                Status (EFICALL *f3[3])       (Pci_io_protocol const *);

                uint64_t    rom_size;
                void const *rom_image;
        };

        static_assert (__is_standard_layout (Pci_io_protocol) && alignof (Pci_io_protocol) == 8 && sizeof (Pci_io_protocol) == 160);

    public:
        // UEFI information must be in a non-BSS section
        SEC_DATA static inline constinit Info info asm ("uefi_info") {};

        SEC_INIT static void init (handle, uintptr_t, Info &) asm ("uefi_init");
};
