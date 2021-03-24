/*
 * Hypervisor Information Page (HIP): Architecture-Independent Part
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "acpi.hpp"
#include "event.hpp"
#include "hip.hpp"
#include "interrupt.hpp"
#include "kmem.hpp"
#include "multiboot.hpp"
#include "space_dma.hpp"
#include "space_gst.hpp"
#include "space_hst.hpp"
#include "space_obj.hpp"
#include "stc.hpp"
#include "stdio.hpp"
#include "uefi.hpp"

extern Hip MHIP_HVAS;

Hip *Hip::hip { reinterpret_cast<Hip *>(&MHIP_HVAS) };

void Hip::build (uint64_t root_s, uint64_t root_e)
{
    auto const uefi { &Uefi::info };

    signature       = Signature::u32 ("NOVA");
    length          = sizeof (*this);
    nova_p_addr     = Kmem::sym_to_phys (&NOVA_HPAS);
    nova_e_addr     = Multiboot::ea;
    mbuf_p_addr     = 0;
    mbuf_e_addr     = 0;
    root_p_addr     = root_s;
    root_e_addr     = root_e;
    acpi_rsdp_addr  = uefi->rsdp ? uefi->rsdp : ~0ULL;
    uefi_mmap_addr  = uefi->mmap ? uefi->mmap : ~0ULL;
    uefi_mmap_size  = uefi->msiz;
    uefi_desc_size  = uefi->dsiz;
    uefi_desc_vers  = uefi->dver;
    tmr_frq         = Stc::freq;
    sel_num         = Space_obj::selectors;
    sel_hst_arch    = Event::hst_arch;
    sel_hst_nova    = Event::hst_max;
    sel_gst_arch    = Event::gst_arch;
    sel_gst_nova    = Event::gst_max;
    cpu_num         = static_cast<uint16_t>(Cpu::count);
    cpu_bsp         = static_cast<uint16_t>(Cpu::id);
    int_pin         = static_cast<uint16_t>(Interrupt::num_pin());
    int_msi         = static_cast<uint16_t>(Interrupt::num_msi());
    mco_obj         = static_cast<uint8_t>(Space_obj::max_order);
    mco_hst         = static_cast<uint8_t>(Space_hst::max_order());
    mco_gst         = static_cast<uint8_t>(Space_gst::max_order());
    mco_dma         = static_cast<uint8_t>(Space_dma::max_order());
    mco_pio         = 16;
    mco_msr         = 16;
    kimax           = static_cast<uint16_t>(Memattr::kimax);

    trace (TRACE_ROOT, "INFO: NOVA: %#018lx-%#018lx", nova_p_addr, nova_e_addr);
    trace (TRACE_ROOT, "INFO: MBUF: %#018lx-%#018lx", mbuf_p_addr, mbuf_e_addr);
    trace (TRACE_ROOT, "INFO: ROOT: %#018lx-%#018lx", root_p_addr, root_e_addr);
    trace (TRACE_ROOT, "INFO: ACPI: %#lx", acpi_rsdp_addr);
    trace (TRACE_ROOT, "INFO: UEFI: %#lx %u %u %u", uefi_mmap_addr, uefi_mmap_size, uefi_desc_size, uefi_desc_vers);
    trace (TRACE_ROOT, "INFO: FREQ: %lu Hz", tmr_frq);
    trace (TRACE_ROOT, "INFO: SEL#: %#lx", sel_num);
    trace (TRACE_ROOT, "INFO: HST#: %3u + %u", sel_hst_arch, sel_hst_nova);
    trace (TRACE_ROOT, "INFO: GST#: %3u + %u", sel_gst_arch, sel_gst_nova);
    trace (TRACE_ROOT, "INFO: CPU#: %3u", cpu_num);
    trace (TRACE_ROOT, "INFO: INT#: %3u + %u", int_pin, int_msi);
    trace (TRACE_ROOT, "INFO: KEY#: %3u", kimax);

    arch.build();

    checksum = 0 - Checksum::additive (reinterpret_cast<uint16_t const *>(this), sizeof (*this) / sizeof (uint16_t));
}
