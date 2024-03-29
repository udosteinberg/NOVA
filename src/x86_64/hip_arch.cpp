/*
 * Hypervisor Information Page (HIP): Architecture-Specific Part (x86)
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

#include "hip_arch.hpp"
#include "tpm_log.hpp"

void Hip_arch::build()
{
    elog_phys = Tpm_log::phys;
    elog_size = Tpm_log::size;
    elog_offs = Tpm_log::offs;
}
