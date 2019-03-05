/*
 * External Symbols
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

#include "types.hpp"

extern char __init_psci[], __init_spin[], GIT_VER[];

extern uintptr_t __boot_p0, __boot_p1, __boot_p2, __boot_ts;

extern uintptr_t PTAB_HVAS;

extern void (*CTORS_L)();
extern void (*CTORS_C)();
extern void (*CTORS_G)();
extern void (*CTORS_E)();
