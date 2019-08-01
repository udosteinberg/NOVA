/*
 * Board-Specific Memory Layout: QEMU Virtual Board
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

#define LOAD_ADDR       0x48000000

#define CL0_CORES       4
#define CL1_CORES       0
#define SPIN_ADDR       0x0
#define SPIN_INCR       0x0

#define HTIMER_PPI      10
#define HTIMER_FLG      0x4

#define VTIMER_PPI      11
#define VTIMER_FLG      0x4

#define SMMU_SPI        0xffffffff
#define SMMU_FLG        0x0

#define SMMU_BASE       0x0
#define SMMU_SIZE       0x0

#define GICD_BASE       0x8000000
#define GICD_SIZE       0x10000

#define GICR_BASE       0x80a0000
#define GICR_SIZE       0xf60000

#define GICC_BASE       0x8010000
#define GICC_SIZE       0x10000

#define GICH_BASE       0x8030000
#define GICH_SIZE       0x10000

#define UART_BASE_CD    0x0
#define UART_BASE_IM    0x0
#define UART_BASE_MI    0x0
#define UART_BASE_PL    0x9000000
#define UART_BASE_SC    0x0
