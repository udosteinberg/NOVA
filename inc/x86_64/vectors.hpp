/*
 * Interrupt Vectors
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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

#include "config.hpp"

#define NUM_FLT         1
#define NUM_IPI         2
#define NUM_LVT         4
#define NUM_GSI         (NUM_VEC - NUM_EXC - NUM_FLT - NUM_IPI - NUM_LVT)

#define VEC_GSI         NUM_EXC
#define VEC_LVT         (VEC_GSI + NUM_GSI)
#define VEC_IPI         (VEC_LVT + NUM_LVT)
#define VEC_FLT         (VEC_IPI + NUM_IPI)
