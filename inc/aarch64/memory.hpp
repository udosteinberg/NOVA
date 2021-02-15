/*
 * Virtual-Memory Layout
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

#include "board.hpp"
#include "macros.hpp"

#define PAGE_BITS       12
#define PAGE_SIZE       BITN (PAGE_BITS)
#define OFFS_MASK       (PAGE_SIZE - 1)

#define LINK_ADDR       LOAD_ADDR
#define OFFSET          (LINK_ADDR - LOAD_ADDR)
