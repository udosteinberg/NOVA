/*
 * Hardware Interface (HW)
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

#include "hw.hpp"
#include "regs.hpp"
#include "smc.hpp"
#include "stdio.hpp"

Status Hw::ctrl (unsigned op, Sys_regs *r)
{
    if (EXPECT_FALSE (op != 0xf)) {
        trace (TRACE_ERROR, "%s: Bad OP (%#x)", __func__, op);
        return Status::BAD_PAR;
    }

    auto arg = uint32 (r->r[1]);
    auto srv = Smc::srv (arg);
    auto fun = Smc::fun (arg);

    // XXX: More sophisticated SMC filtering

    if (EXPECT_FALSE (srv != Smc::Service::SIP || !(arg & BIT (31)))) {
        trace (TRACE_ERROR, "%s: Bad SMC (%#x)", __func__, arg);
        return Status::BAD_PAR;
    }

    if (EXPECT_FALSE (!Cpu::feature (Cpu::Cpu_feature::EL3))) {
        trace (TRACE_ERROR, "%s: SMC not supported", __func__);
        return Status::BAD_FTR;
    }

    r->r[1] = (arg & BIT (30) ? Smc::call<uint64> : Smc::call<uint32>)(srv, fun, r->r[2], r->r[3], r->r[4], r->r[5], r->r[6], r->r[7], 0);

    return Status::SUCCESS;
}
