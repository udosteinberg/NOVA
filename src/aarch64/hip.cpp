/*
 * Hypervisor Information Page (HIP)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "console_mbuf.hpp"
#include "event.hpp"
#include "extern.hpp"
#include "gicd.hpp"
#include "hip.hpp"
#include "memory.hpp"
#include "smmu.hpp"
#include "space_obj.hpp"
#include "stdio.hpp"

Hip *Hip::hip = reinterpret_cast<Hip *>(&PAGEH);

void Hip::build (uint64 root_s, uint64 root_e)
{
    signature       = 0x41564f4e;
    length          = sizeof (*this);
    nova_s_addr     = LOAD_ADDR;
    nova_e_addr     = reinterpret_cast<uint64>(&LOAD_STOP);
    mbuf_s_addr     = Console_mbuf::con.addr();
    mbuf_e_addr     = Console_mbuf::con.addr() + Console_mbuf::con.size();
    root_s_addr     = root_s;
    root_e_addr     = root_e;
    sel_num         = Space_obj::num;
    sel_hst_arch    = Event::hst_arch;
    sel_hst_nova    = Event::hst_max;
    sel_gst_arch    = Event::gst_arch;
    sel_gst_nova    = Event::gst_max;
    cpu_num         = static_cast<uint16>(Cpu::online);
    int_num         = static_cast<uint16>(Gicd::ints - 32);
    smg_num         = static_cast<uint16>(Smmu::smrg);
    ctx_num         = static_cast<uint16>(Smmu::ctxb);

    uint16 c = 0;
    for (uint16 const *ptr = reinterpret_cast<uint16 const *>(this);
                       ptr < reinterpret_cast<uint16 const *>(this + 1);
                       c = static_cast<uint16>(c - *ptr++)) ;

    checksum = c;

    trace (TRACE_ROOT, "INFO: NOVA: %#010llx-%#010llx", nova_s_addr, nova_e_addr);
    trace (TRACE_ROOT, "INFO: MBUF: %#010llx-%#010llx", mbuf_s_addr, mbuf_e_addr);
    trace (TRACE_ROOT, "INFO: ROOT: %#010llx-%#010llx", root_s_addr, root_e_addr);
    trace (TRACE_ROOT, "INFO: SEL#: %llu", sel_num);
    trace (TRACE_ROOT, "INFO: HST#: %u + %u", sel_hst_arch, sel_hst_nova);
    trace (TRACE_ROOT, "INFO: GST#: %u + %u", sel_gst_arch, sel_gst_nova);
    trace (TRACE_ROOT, "INFO: CPU#: %u", cpu_num);
    trace (TRACE_ROOT, "INFO: INT#: %u", int_num);
    trace (TRACE_ROOT, "INFO: SMG#: %u", smg_num);
    trace (TRACE_ROOT, "INFO: CTX#: %u", ctx_num);
}
