/*
 * Virtual-Memory Layout
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#define PAGE_BITS       12
#define PAGE_SIZE       (1 << PAGE_BITS)
#define PAGE_MASK       (PAGE_SIZE - 1)

#define LOAD_ADDR       0x200000
#define USER_ADDR       0xc0000000

#define LINK_ADDR       0xc0000000

// Global Range from 0xc0000000 to 0xcfc00000
#define CPUGL_ADDR      0xcc000000
#define HWDEV_EADDR     0xcfbff000
#define VGACN_ADDR      0xcfbff000

// CPU Local Range from 0xcfc00000 to 0xd0000000
#define LOCAL_SADDR     0xcfc00000
#define LAPIC_ADDR      0xcfffc000
#define KSTCK_ADDR      0xcfffe000
#define CPULC_ADDR      0xcffff000

// AS Local Range from 0xd0000000 to max
#define IOBMP_SADDR     0xd0000000
#define IOBMP_EADDR     (IOBMP_SADDR + PAGE_SIZE * 2)
#define REMAP_SADDR     0xdf000000
#define OBJSP_SADDR     0xe0000000
#define OBJSP_EADDR     (OBJSP_SADDR + 0x20000000)
