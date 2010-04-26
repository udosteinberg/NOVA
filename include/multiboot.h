/*
 * Multiboot Support
 *
 * Author: Udo Steinberg <udo@hypervisor.org>
 * TU Dresden, Operating Systems Group
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

#include "compiler.h"

/*
 * Multiboot Information Structure
 */
class Multiboot
{
    public:
        enum
        {
            MAGIC           = 0x2badb002,
            MEMORY          = 1ul << 0,
            BOOT_DEVICE     = 1ul << 1,
            CMDLINE         = 1ul << 2,
            MODULES         = 1ul << 3,
            SYMBOLS         = 1ul << 4 | 1ul << 5,
            MEMORY_MAP      = 1ul << 6,
            DRIVES          = 1ul << 7,
            CONFIG_TABLE    = 1ul << 8,
            LOADER_NAME     = 1ul << 9,
            APM_TABLE       = 1ul << 10,
            VBE_INFO        = 1ul << 11
        };

        uint32  flags;              // 0
        uint32  mem_lower;          // 4
        uint32  mem_upper;          // 8
        uint32  boot_device;        // 12
        uint32  cmdline;            // 16
        uint32  mods_count;         // 20
        uint32  mods_addr;          // 24
        uint32  syms[4];            // 28,32,36,40
        uint32  mmap_len;           // 44
        uint32  mmap_addr;          // 48
        uint32  drives_length;      // 52
        uint32  drives_addr;        // 56
        uint32  config_table;       // 60
        uint32  loader_name;        // 64
};

/*
 * Multiboot Module
 */
class Multiboot_module
{
    public:
        uint32  s_addr;
        uint32  e_addr;
        uint32  cmdline;
        uint32  reserved;
};

/*
 * Multiboot Memory Map
 */
#pragma pack(1)
class Multiboot_mmap
{
    public:
        uint32  size;
        uint64  addr;
        uint64  len;
        uint32  type;
};
#pragma pack()
