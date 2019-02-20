/*
 * External Symbols
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#include "types.hpp"

extern char GIT_VER;

extern char PAGE_0;
extern char PAGE_1;
extern char PAGE_H;

extern uintptr_t NOVA_HPAS, NOVA_HPAE, PTAB_HVAS;

extern void (*CTORS_L)();
extern void (*CTORS_C)();
extern void (*CTORS_G)();
extern void (*CTORS_E)();

extern char __init_aps, __desc_gdt__;
extern char entry_sysenter;
extern char entry_vmx;
extern mword handlers[];
extern mword hwdev_addr;
