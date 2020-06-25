/*
 * Segment Selectors
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

#define SEL_KERN_CODE   0x8     // IA32_STAR[47:32] + 0
#define SEL_KERN_DATA   0x10    // IA32_STAR[47:32] + 8
#define SEL_USER_DATA   0x1b    // IA32_STAR[63:48] + 8
#define SEL_USER_CODE   0x23    // IA32_STAR[63:48] + 16
#define SEL_TSS_RUN     0x30
#define SEL_TSS_DBF     0x40
#define SEL_MAX         0x50
