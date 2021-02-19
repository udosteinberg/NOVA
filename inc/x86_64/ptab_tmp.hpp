/*
 * Page Table Templates
 *
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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

#include "ptab_dpt.hpp"
#include "ptab_ept.hpp"
#include "ptab_hpt.hpp"

template class Ptab<Dpt, uint64_t, uint64_t>;
template class Ptab<Ept, uint64_t, uint64_t>;
template class Ptab<Hpt, uint64_t, uint64_t>;
