/*
 * Interrupt Vectors
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

#include "config.hpp"

#define NUM_SVI         1
#define NUM_FLT         1
#define NUM_LVT         5
#define NUM_IPI         2
#define NUM_GSI         (NUM_VEC - NUM_SVI - NUM_FLT - NUM_LVT - NUM_IPI - NUM_EXC)

#define VEC_SVI         (VEC_FLT + NUM_FLT)
#define VEC_FLT         (VEC_LVT + NUM_LVT)
#define VEC_LVT         (VEC_IPI + NUM_IPI)
#define VEC_IPI         (VEC_GSI + NUM_GSI)
#define VEC_GSI         NUM_EXC
