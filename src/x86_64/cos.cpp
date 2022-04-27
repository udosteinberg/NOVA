/*
 * Class Of Service (COS)
 * Code/Data Prioritization (CDP)
 * Cache Allocation Technology (CAT)
 * Memory Bandwidth Allocation (MBA)
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

#include "cos.hpp"
#include "stdio.hpp"

uint8_t  Cos::config { 0 }, Cos::supcfg { 0 }, Cos::cos_l3 { 0 }, Cos::cos_l2 { 0 }, Cos::cos_mb { 0 }, Cos::hcb_l3 { 0 }, Cos::hcb_l2 { 0 };
uint16_t Cos::del_mb { 0 }, Cos::current { 0 };

/*
 * Initialize COS, CAT, MBA to default values
 */
void Cos::init()
{
    if (!Cpu::feature (Cpu::Feature::RDT_A))
        return;

    trace (TRACE_CPU, "RDTA: L3:%u L2:%u MB:%u", cos_l3, cos_l2, cos_mb);

    for (uint8_t i { 0 }; i < cos_l3; i++)
        set_l3_mask (i, BIT_RANGE (hcb_l3, 0));

    for (uint8_t i { 0 }; i < cos_l2; i++)
        set_l2_mask (i, BIT_RANGE (hcb_l2, 0));

    for (uint8_t i { 0 }; i < cos_mb; i++)
        set_mb_thrt (i, 0);

    /*
     * Before enabling/disabling CDP (below), software should write 1's to
     * all CAT/CDP masks (above) to ensure proper behavior. Upon resume, we
     * restore Cos::config and Cos::current to their prior suspend values,
     * to ensure consistency between CDP state and the COS values in
     * existing kernel objects.
     */

    set_qos_cfg (config);

    set_cos (current);
}
