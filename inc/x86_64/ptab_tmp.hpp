/*
 * Page Table Templates
 *
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

#include "ptab_dma_amd.hpp"
#include "ptab_dma_itl.hpp"
#include "ptab_gst_amd.hpp"
#include "ptab_gst_itl.hpp"
#include "ptab_hst.hpp"

template class Ptab<Pte_dma_amd, uint64_t, uint64_t>;
template class Ptab<Pte_dma_itl, uint64_t, uint64_t>;
template class Ptab<Pte_gst_amd, uint64_t, uint64_t>;
template class Ptab<Pte_gst_itl, uint64_t, uint64_t>;
template class Ptab<Pte_hst,     uint64_t, uint64_t>;
