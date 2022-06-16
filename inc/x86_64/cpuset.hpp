/*
 * Atomic CPU Set
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

#include "bitmap.hpp"
#include "config.hpp"

/*
 * CPU A (marking TLB dirty for EC X->HST)      CPU B (switching to EC X)
 *
 * (1) ST.SEQ_CST (X->HST->cpuset = dirty)      (3) ST.SEQ_CST (Ec::current = X)
 * (2) LD.SEQ_CST (Ec::current)                 (4) LD.SEQ_CST (X->HST->cpuset)
 *
 * Required Memory Ordering
 *
 * (1) happens before (2)                       (3) happens before (4)
 * (2) synchronizes with (3)                    (4) synchronizes with (1)
 */
using Cpuset = Bitmap<NUM_CPU, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST>;
