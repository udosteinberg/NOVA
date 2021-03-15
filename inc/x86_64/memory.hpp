/*
 * Virtual-Memory Layout
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#define LOAD_ADDR       0x400000

#define LINK_ADDR       0xffffffff81000000
#define CPU_LOCAL       0xffffffffbfe00000
#define SPC_LOCAL       0xffffffffc0000000

#define OFFSET          (LINK_ADDR - LOAD_ADDR)

#define CPU_GLOBL_DATA  (CPU_LOCAL - 0x1000000)

#define CPU_LOCAL_STCK  (SPC_LOCAL - PAGE_SIZE * 3)
#define CPU_LOCAL_APIC  (SPC_LOCAL - PAGE_SIZE * 2)
#define CPU_LOCAL_DATA  (SPC_LOCAL - PAGE_SIZE * 1)

#define SPC_LOCAL_IOP   (SPC_LOCAL)
#define SPC_LOCAL_IOP_E (SPC_LOCAL_IOP + PAGE_SIZE * 2)
#define SPC_LOCAL_REMAP (SPC_LOCAL_OBJ - 0x1000000)
#define SPC_LOCAL_OBJ   (END_SPACE_LIM - 0x20000000)

#define END_SPACE_LIM   (~0UL + 1)
