/*
 * Hypervisor Information Page (HIP): Architecture-Independent Part
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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
#include "space_msr.hpp"
#include "space_obj.hpp"
#include "space_pio.hpp"
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
    sbw_obj         = Space_obj::sbw;
    sbw_hst         = Space_hst::sbw;
    sbw_gst         = Space_gst::sbw;
    sbw_dma         = Space_dma::sbw;
    sbw_pio         = Space_pio::sbw;
    sbw_msr         = Space_msr::sbw;
    cpu_bsp         = Cpu::id;
    mco_obj         = Space_obj::mco;
    mco_hst         = Space_hst::mco();
    mco_gst         = Space_gst::mco();
    mco_dma         = Space_dma::mco();
    mco_pio         = Space_pio::mco;
    mco_msr         = Space_msr::mco;
    gsi_pin         = Interrupt::gsi_pin;
    kid_max         = static_cast<uint16_t>(Memattr::kimax);
    gsi_max         = Interrupt::gsi_max;
    cpu_max         = Cpu::count - 1;
    vec_max         = Interrupt::vec_max;
    sel_hst_arch    = Event::hst_arch;
    sel_hst_nova    = Event::hst_max;
    sel_gst_arch    = Event::gst_arch;
    sel_gst_nova    = Event::gst_max;

    trace (TRACE_ROOT, "INFO: NOVA: %#018lx-%#018lx", nova_p_addr, nova_e_addr);
    trace (TRACE_ROOT, "INFO: MBUF: %#018lx-%#018lx", mbuf_p_addr, mbuf_e_addr);
    trace (TRACE_ROOT, "INFO: ROOT: %#018lx-%#018lx", root_p_addr, root_e_addr);
    trace (TRACE_ROOT, "INFO: ACPI: %#lx", acpi_rsdp_addr);
    trace (TRACE_ROOT, "INFO: UEFI: %#lx %u %u %u", uefi_mmap_addr, uefi_mmap_size, uefi_desc_size, uefi_desc_vers);
    trace (TRACE_ROOT, "INFO: FREQ: %lu Hz", tmr_frq);
    trace (TRACE_ROOT, "INFO: SBW#: OBJ:%u HST:%u GST:%u DMA:%u PIO:%u MSR:%u", sbw_obj, sbw_hst, sbw_gst, sbw_dma, sbw_pio, sbw_msr);
    trace (TRACE_ROOT, "INFO: HST#: %3u + %u", sel_hst_arch, sel_hst_nova);
    trace (TRACE_ROOT, "INFO: GST#: %3u + %u", sel_gst_arch, sel_gst_nova);
    trace (TRACE_ROOT, "INFO: CPU#: %5u", cpu_max + 1);
    trace (TRACE_ROOT, "INFO: GSI#: %5u (%u)", gsi_max + 1, gsi_pin);

    arch.build();

    checksum = 0 - Checksum::additive (reinterpret_cast<uint16_t const *>(this), sizeof (*this) / sizeof (uint16_t));
}
