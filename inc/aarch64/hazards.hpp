/*
 * Hazards
 *
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

#define HZD_SCHED       0x1
#define HZD_RCU         0x10
#define HZD_BOOT_HST    0x20
#define HZD_BOOT_GST    0x40
#define HZD_FPU         0x10000
#define HZD_ILLEGAL     0x40000000
#define HZD_RECALL      0x80000000
