/*
 * Page Table Entry (PTE)
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

#include "dpt.hpp"
#include "ept.hpp"
#include "hpt.hpp"

template<> unsigned Pte<Dpt, 4, 9, uint64, uint64>::lim = ~0U;
template<> unsigned Pte<Ept, 4, 9, uint64, uint64>::lim = ~0U;
template<> unsigned Pte<Hpt, 4, 9, uint64, uint64>::lim = ~0U;

template class Pte<Dpt, 4, 9, uint64, uint64>;
template class Pte<Ept, 4, 9, uint64, uint64>;
template class Pte<Hpt, 4, 9, uint64, uint64>;
