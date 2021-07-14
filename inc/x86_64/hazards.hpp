/*
 * Hazards
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "macros.hpp"

#define HZD_SCHED       BIT  (0)
#define HZD_SLEEP       BIT  (1)
#define HZD_RCU         BIT  (2)
#define HZD_BOOT_HST    BIT  (8)
#define HZD_BOOT_GST    BIT  (9)
#define HZD_DS_ES       BIT (14)
#define HZD_TR          BIT (15)
#define HZD_FPU         BIT (16)
#define HZD_TSC         BIT (29)
#define HZD_RECALL      BIT (30)
#define HZD_ILLEGAL     BIT (31)
