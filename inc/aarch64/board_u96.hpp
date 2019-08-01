/*
 * Board-Specific Memory Layout: Avnet Ultra96
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

#define LOAD_ADDR       0x7f000000
#define ROOT_ADDR       0x7ee00000
#define FDTB_ADDR       0x7def4be8

#define CL0_CORES       4
#define CL1_CORES       0
#define SPIN_ADDR       0x0
#define SPIN_INCR       0x0

#define HTIMER_PPI      10
#define HTIMER_FLG      0x8

#define VTIMER_PPI      11
#define VTIMER_FLG      0x8

#define SMMU_SPI        0x9b
#define SMMU_FLG        0x4

#define SMMU_BASE       0xfd800000
#define SMMU_SIZE       0x20000

#define GICD_BASE       0xf9010000
#define GICD_SIZE       0x10000

#define GICR_BASE       0x0
#define GICR_SIZE       0x0

#define GICC_BASE       0xf9020000
#define GICC_SIZE       0x20000

#define GICH_BASE       0xf9040000
#define GICH_SIZE       0x20000

#define UART_BASE_CD    0xff010000
#define UART_BASE_IM    0x0
#define UART_BASE_MI    0x0
#define UART_BASE_PL    0x0
#define UART_BASE_SC    0x0
