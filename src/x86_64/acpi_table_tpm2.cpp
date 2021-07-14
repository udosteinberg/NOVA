/*
 * Advanced Configuration and Power Interface (ACPI)
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

#include "acpi_table_tpm2.hpp"
#include "ptab_hpt.hpp"
#include "stdio.hpp"

bool Acpi_table_tpm2::parse() const
{
    if (start_method == 2) {

        // Map CRB Control Area
        auto const m { Hptp::map_tmp (ctrl_area, sizeof (Control), Paging::R, Memattr::dev(), 1) };
        if (!m) [[unlikely]]
            return false;

        auto const ctrl { std::start_lifetime_as<Control const> (m) };

        trace (TRACE_TPM, "TPM2: CTRL:%#lx CMD:%#lx/%#x RSP:%#lx/%#x", uint64_t { ctrl_area },
               uint64_t { ctrl->cmd_addr }, uint32_t { ctrl->cmd_size },
               uint64_t { ctrl->rsp_addr }, uint32_t { ctrl->rsp_size });
    }

    return true;
}
