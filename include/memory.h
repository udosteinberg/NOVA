/*
 * Virtual-Memory Layout
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#ifdef __i386__

#define LINK_ADDR       0xc0000000
#define CPUGL_ADDR      0xcc000000
#define HWDEV_EADDR     0xcfbff000
#define VGACN_ADDR      0xcfbff000

#define LOCAL_SADDR     0xcfc00000
#define LAPIC_ADDR      0xcfffc000
#define KSTCK_ADDR      0xcfffe000
#define CPULC_ADDR      0xcffff000

#define IOBMP_SADDR     0xd0000000
#define IOBMP_EADDR     (IOBMP_SADDR + PAGE_SIZE * 2)
#define REMAP_SADDR     0xdf000000
#define OBJSP_SADDR     0xe0000000
#define OBJSP_EADDR     (OBJSP_SADDR + 0x20000000)

#else

#define LINK_ADDR       0xffffffffc0000000
#define CPUGL_ADDR      0xffffffffcc000000
#define HWDEV_EADDR     0xffffffffcfbff000
#define VGACN_ADDR      0xffffffffcfbff000

#define LOCAL_SADDR     0xffffffffcfc00000
#define LAPIC_ADDR      0xffffffffcfffc000
#define KSTCK_ADDR      0xffffffffcfffe000
#define CPULC_ADDR      0xffffffffcffff000

#define IOBMP_SADDR     0xffffff0000000000
#define IOBMP_EADDR     (IOBMP_SADDR + PAGE_SIZE * 2)
#define REMAP_SADDR     0xffffff1000000000
#define OBJSP_SADDR     0xffffff2000000000
#define OBJSP_EADDR     (OBJSP_SADDR + 0x20000000)

#endif
