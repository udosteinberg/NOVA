/*
 * Secure Monitor Calls (SMC)
 *
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "smc.hpp"
#include "stdio.hpp"

void Smc::init()
{
    // XXX: Check if EL3 is implemented

    mword p0, p1, p2, p3;

    p0 = call<uint32> (Service::ARM, Function::UID, p1, p2, p3);
    if (p0 != ~0UL)
        trace (TRACE_FIRM, "UUID: ARM %08lx-%08lx-%08lx-%08lx", p0, p1, p2, p3);

    p0 = call<uint32> (Service::CPU, Function::UID, p1, p2, p3);
    if (p0 != ~0UL)
        trace (TRACE_FIRM, "UUID: CPU %08lx-%08lx-%08lx-%08lx", p0, p1, p2, p3);

    p0 = call<uint32> (Service::SIP, Function::UID, p1, p2, p3);
    if (p0 != ~0UL)
        trace (TRACE_FIRM, "UUID: SIP %08lx-%08lx-%08lx-%08lx", p0, p1, p2, p3);

    p0 = call<uint32> (Service::OEM, Function::UID, p1, p2, p3);
    if (p0 != ~0UL)
        trace (TRACE_FIRM, "UUID: OEM %08lx-%08lx-%08lx-%08lx", p0, p1, p2, p3);

    p0 = call<uint32> (Service::SEC, Function::UID, p1, p2, p3);
    if (p0 != ~0UL)
        trace (TRACE_FIRM, "UUID: SEC %08lx-%08lx-%08lx-%08lx", p0, p1, p2, p3);

    p0 = call<uint32> (Service::HYP, Function::UID, p1, p2, p3);
    if (p0 != ~0UL)
        trace (TRACE_FIRM, "UUID: HYP %08lx-%08lx-%08lx-%08lx", p0, p1, p2, p3);

    p0 = call<uint32> (Service::TOS, Function::UID, p1, p2, p3);
    if (p0 != ~0UL)
        trace (TRACE_FIRM, "UUID: TOS %08lx-%08lx-%08lx-%08lx", p0, p1, p2, p3);
}
