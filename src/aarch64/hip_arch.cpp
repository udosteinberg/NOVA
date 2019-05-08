/*
 * Hypervisor Information Page (HIP): Architecture-Specific Part (ARM)
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

#include "fdt.hpp"
#include "hip_arch.hpp"
#include "smmu.hpp"
#include "stdio.hpp"

void Hip_arch::build()
{
    fdt_addr    = Fdt::phys;
    num_smg     = static_cast<uint16>(Smmu::num_smg);
    num_ctx     = static_cast<uint16>(Smmu::num_ctx);

    trace (TRACE_ROOT, "INFO: FDT*: %#llx", fdt_addr);
    trace (TRACE_ROOT, "INFO: SMG#: %3u", num_smg);
    trace (TRACE_ROOT, "INFO: CTX#: %3u", num_ctx);
}
