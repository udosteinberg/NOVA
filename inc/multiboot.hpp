/*
 * Multiboot Definitions (v1, v2)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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

#define MULTIBOOT_V1_HEADER     0x1badb002
#define MULTIBOOT_V1_LOADER     0x2badb002

#define MULTIBOOT_V1_INFO_CMD   2
#define MULTIBOOT_V1_INFO_MOD   3

#define MULTIBOOT_V2_HEADER     0xe85250d6
#define MULTIBOOT_V2_LOADER     0x36d76289

#define MULTIBOOT_V2_INFO_END   0
#define MULTIBOOT_V2_INFO_CMD   1
#define MULTIBOOT_V2_INFO_MOD   3
#define MULTIBOOT_V2_INFO_SYS   12
#define MULTIBOOT_V2_INFO_IMG   20
#define MULTIBOOT_V2_INFO_KMEM  0x4d454d4b

#ifndef __ASSEMBLER__

#include "compiler.hpp"
#include "types.hpp"

class Multiboot final
{
    public:
        // Multiboot parameters must be in a non-BSS section
        SEC_DATA static inline uintptr_t cl asm ("multiboot_cl")  { 0 };
        SEC_DATA static inline uintptr_t ea asm ("multiboot_ea")  { 0 };
        SEC_DATA static inline uintptr_t ra asm ("multiboot_ra")  { 0 };

        SEC_DATA static inline uintptr_t p0 asm ("multiboot_p0")  { 0 };
        SEC_DATA static inline uintptr_t p1 asm ("multiboot_p1")  { 0 };
        SEC_DATA static inline uintptr_t p2 asm ("multiboot_p2")  { 0 };

        SEC_DATA static inline uint64_t  t0 asm ("multiboot_t0")  { 0 };
        SEC_DATA static inline uint64_t  t1 asm ("multiboot_t1")  { 0 };
        SEC_DATA static inline uint64_t  t2 asm ("multiboot_t2")  { 0 };
};

#endif
