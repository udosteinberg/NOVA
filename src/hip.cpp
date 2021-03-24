/*
 * Hypervisor Information Page (HIP): Architecture-Independent Part
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "acpi.hpp"
#include "console_mbuf.hpp"
#include "event.hpp"
#include "extern.hpp"
#include "hip.hpp"
#include "interrupt.hpp"
#include "kmem.hpp"
#include "memory.hpp"
#include "space_obj.hpp"
#include "stdio.hpp"
#include "timer.hpp"
#include "uefi.hpp"

Hip *Hip::hip = reinterpret_cast<Hip *>(&MHIP_HVAS);

void Hip::build (uint64 root_s, uint64 root_e)
{
    auto uefi = reinterpret_cast<Uefi *>(Kmem::sym_to_virt (&Uefi::uefi));

    signature       = 0x41564f4e;
    length          = sizeof (*this);
    nova_p_addr     = Kmem::sym_to_phys (&NOVA_HPAS);
    nova_e_addr     = Kmem::sym_to_phys (&NOVA_HPAE);
    mbuf_p_addr     = Console_mbuf::addr();
    mbuf_e_addr     = Console_mbuf::size() + mbuf_p_addr;
    root_p_addr     = root_s;
    root_e_addr     = root_e;
    acpi_rsdp_addr  = uefi->rsdp ? uefi->rsdp : ~0ULL;
    uefi_mmap_addr  = uefi->mmap ? uefi->mmap : ~0ULL;
    uefi_mmap_size  = uefi->msiz;
    uefi_desc_size  = uefi->dsiz;
    uefi_desc_vers  = uefi->dver;
    tmr_frq         = Timer::ms_to_ticks (1000);
    sel_num         = Space_obj::num;
    sel_hst_arch    = Event::hst_arch;
    sel_hst_nova    = Event::hst_max;
    sel_gst_arch    = Event::gst_arch;
    sel_gst_nova    = Event::gst_max;
    num_cpu         = static_cast<uint16>(Cpu::online);
    num_int         = static_cast<uint16>(Interrupt::count());

    trace (TRACE_ROOT, "INFO: NOVA: %#018llx-%#018llx", nova_p_addr, nova_e_addr);
    trace (TRACE_ROOT, "INFO: MBUF: %#018llx-%#018llx", mbuf_p_addr, mbuf_e_addr);
    trace (TRACE_ROOT, "INFO: ROOT: %#018llx-%#018llx", root_p_addr, root_e_addr);
    trace (TRACE_ROOT, "INFO: ACPI: %#llx", acpi_rsdp_addr);
    trace (TRACE_ROOT, "INFO: UEFI: %#llx %u %u %u", uefi_mmap_addr, uefi_mmap_size, uefi_desc_size, uefi_desc_vers);
    trace (TRACE_ROOT, "INFO: FREQ: %llu Hz", tmr_frq);
    trace (TRACE_ROOT, "INFO: SEL#: %#llx", sel_num);
    trace (TRACE_ROOT, "INFO: HST#: %3u + %u", sel_hst_arch, sel_hst_nova);
    trace (TRACE_ROOT, "INFO: GST#: %3u + %u", sel_gst_arch, sel_gst_nova);
    trace (TRACE_ROOT, "INFO: CPU#: %3u", num_cpu);
    trace (TRACE_ROOT, "INFO: INT#: %3u", num_int);

    arch.build();

    uint16 c = 0;
    for (uint16 const *ptr = reinterpret_cast<uint16 const *>(this);
                       ptr < reinterpret_cast<uint16 const *>(this + 1);
                       c = static_cast<uint16>(c - *ptr++)) ;

    checksum = c;
}
