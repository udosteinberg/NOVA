/*
 * External Symbols
 *
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

extern uintptr_t GIT_VER, NOVA_HPAS, NOVA_HPAE, MHIP_HVAS, PTAB_HVAS, STCK_TOP;
extern uintptr_t __boot_cl, __boot_ra, __boot_p0, __boot_p1, __boot_p2, __boot_ts, __init_psci;

extern void (*CTORS_L)();
extern void (*CTORS_C)();
extern void (*CTORS_G)();
extern void (*CTORS_E)();
