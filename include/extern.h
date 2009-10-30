/*
 * External Symbols
 *
 * Copyright (C) 2007-2008, Udo Steinberg <udo@hypervisor.org>
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

#include "types.h"

extern char PAGE_0;
extern char PAGE_1;
extern char PAGE_H;

extern char FRAME_0;
extern char FRAME_1;
extern char FRAME_H;

extern char LINK_PHYS;
extern char LINK_SIZE;
extern char LOAD_SIZE;
extern char OFFSET;

extern void (*CTORS_S)();
extern void (*CTORS_E)();

extern uint64 PDP, PDE, PTE;

extern char entry_sysenter;
extern char entry_vmx;
extern mword handlers[];
extern mword hwdev_addr;
