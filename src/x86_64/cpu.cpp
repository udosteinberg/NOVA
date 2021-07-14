/*
 * Central Processing Unit (CPU)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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
#include "bits.hpp"
#include "cache.hpp"
#include "cmdline.hpp"
#include "counter.hpp"
#include "cr.hpp"
#include "gdt.hpp"
#include "hip.hpp"
#include "idt.hpp"
#include "lapic.hpp"
#include "lowlevel.hpp"
#include "mca.hpp"
#include "pd.hpp"
#include "stdio.hpp"
#include "svm.hpp"
#include "tss.hpp"
#include "vmx.hpp"

char const * const Cpu::vendor_string[] =
{
    "Unknown",
    "GenuineIntel",
    "AuthenticAMD"
};

// Order of these matters
unsigned    Cpu::count;
uint8       Cpu::acpi_id[NUM_CPU];
uint8       Cpu::apic_id[NUM_CPU];

unsigned    Cpu::id;
unsigned    Cpu::hazard;
unsigned    Cpu::package;
unsigned    Cpu::core;
unsigned    Cpu::thread;

Cpu::Vendor Cpu::vendor;
unsigned    Cpu::platform;
unsigned    Cpu::family;
unsigned    Cpu::model;
unsigned    Cpu::stepping;
unsigned    Cpu::brand;
unsigned    Cpu::patch;

uint32      Cpu::name[12];
uint32      Cpu::features[6];
bool        Cpu::bsp;

void Cpu::check_features()
{
    unsigned top, tpp = 1, cpp = 1;

    uint32 eax, ebx, ecx, edx;

    cpuid (0, eax, ebx, ecx, edx);

    size_t v;
    for (v = sizeof (vendor_string) / sizeof (*vendor_string); --v;)
        if (*reinterpret_cast<uint32 const *>(vendor_string[v] + 0) == ebx &&
            *reinterpret_cast<uint32 const *>(vendor_string[v] + 4) == edx &&
            *reinterpret_cast<uint32 const *>(vendor_string[v] + 8) == ecx)
            break;

    vendor = Vendor (v);

    if (vendor == INTEL) {
        Msr::write (Msr::Register::IA32_BIOS_SIGN_ID, 0);
        platform = static_cast<unsigned>(Msr::read (Msr::Register::IA32_PLATFORM_ID) >> 50) & 7;
    }

    switch (static_cast<uint8>(eax)) {
        default:
            cpuid (0x7, 0, eax, features[3], ecx, edx);
            [[fallthrough]];
        case 0x6:
            cpuid (0x6, features[2], ebx, ecx, edx);
            [[fallthrough]];
        case 0x4 ... 0x5:
            cpuid (0x4, 0, eax, ebx, ecx, edx);
            cpp = (eax >> 26 & 0x3f) + 1;
            [[fallthrough]];
        case 0x1 ... 0x3:
            cpuid (0x1, eax, ebx, features[1], features[0]);
            family   = (eax >> 8 & 0xf) + (eax >> 20 & 0xff);
            model    = (eax >> 4 & 0xf) + (eax >> 12 & 0xf0);
            stepping =  eax & 0xf;
            brand    =  ebx & 0xff;
            top      =  ebx >> 24;
            tpp      =  ebx >> 16 & 0xff;
            Cache::init (8 * (ebx >> 8 & 0xff));
    }

    patch = static_cast<unsigned>(Msr::read (Msr::Register::IA32_BIOS_SIGN_ID) >> 32);

    cpuid (0x80000000, eax, ebx, ecx, edx);

    if (eax & 0x80000000) {
        switch (static_cast<uint8>(eax)) {
            default:
                cpuid (0x8000000a, Vmcb::svm_version, ebx, ecx, Vmcb::svm_feature);
                [[fallthrough]];
            case 0x4 ... 0x9:
                cpuid (0x80000004, name[8], name[9], name[10], name[11]);
                [[fallthrough]];
            case 0x3:
                cpuid (0x80000003, name[4], name[5], name[6], name[7]);
                [[fallthrough]];
            case 0x2:
                cpuid (0x80000002, name[0], name[1], name[2], name[3]);
                [[fallthrough]];
            case 0x1:
                cpuid (0x80000001, eax, ebx, features[5], features[4]);
        }
    }

    if (feature (FEAT_CMP_LEGACY))
        cpp = tpp;

    unsigned tpc = tpp / cpp;
    unsigned long t_bits = bit_scan_reverse (tpc - 1) + 1;
    unsigned long c_bits = bit_scan_reverse (cpp - 1) + 1;

    thread  = top            & ((1u << t_bits) - 1);
    core    = top >>  t_bits & ((1u << c_bits) - 1);
    package = top >> (t_bits + c_bits);
}

void Cpu::setup_pcid()
{
    if (EXPECT_FALSE (Cmdline::nopcid))
        defeature (FEAT_PCID);

    if (EXPECT_FALSE (!feature (FEAT_PCID)))
        return;

    Cr::set_cr4 (Cr::get_cr4() | CR4_PCIDE);
}

void Cpu::setup_msr()
{
    if (EXPECT_TRUE (feature (FEAT_ACPI)))
        Msr::write (Msr::Register::IA32_THERM_INTERRUPT, 0x10);

    if (EXPECT_TRUE (feature (FEAT_SEP)))
        Msr::write (Msr::Register::IA32_SYSENTER_CS, 0);

    if (EXPECT_TRUE (feature (FEAT_LM))) {
        Msr::write (Msr::Register::IA32_STAR,  hstate.star);
        Msr::write (Msr::Register::IA32_LSTAR, hstate.lstar);
        Msr::write (Msr::Register::IA32_FMASK, hstate.fmask);
        Msr::write (Msr::Register::IA32_KERNEL_GS_BASE, hstate.kernel_gs_base);
    }

    // Disable C1E on AMD Rev.F and beyond because it stops LAPIC clock
    if (vendor == Vendor::AMD)
        if (family > 0xf || (family == 0xf && model >= 0x40))
            Msr::write (Msr::Register::AMD_IPMR, Msr::read (Msr::Register::AMD_IPMR) & ~(3UL << 27));
}

void Cpu::init()
{
    uint32 clk { 0 }, rat { 0 };

    for (void (**func)() = &CTORS_L; func != &CTORS_C; (*func++)()) ;

    Gdt::build();
    Tss::build();

    // Initialize exception handling
    Gdt::load();
    Tss::load();
    Idt::load();

    // Initialize CPU number and check features
    check_features();

    Lapic::init (clk, rat);

    uint64 phys; unsigned o; Memattr::Cacheability ca; Memattr::Shareability sh;
    Pd::kern.Space_hst::loc[id] = Hptp::current();
    Pd::kern.Space_hst::loc[id].lookup (MMAP_CPU_DATA, phys, o, ca, sh);
    Hptp::master_map (MMAP_GLB_DATA + id * PAGE_SIZE, phys, 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), ca, sh);
    Hptp::set_leaf_max (feature (FEAT_1GB_PAGES) ? 3 : 2);

    setup_msr();

    setup_pcid();

    Cr::set_cr4 (Cr::get_cr4() | feature (FEAT_SMAP) * CR4_SMAP  |
                                 feature (FEAT_SMEP) * CR4_SMEP  |
                                 feature (FEAT_MCE)  * CR4_MCE);

    Vmcs::init();
    Vmcb::init();

    Mca::init();

    trace (TRACE_CPU, "CORE:%x:%x:%x %x:%x:%x:%x [%x] %.48s", package, core, thread, family, model, stepping, platform, patch, reinterpret_cast<char *>(name));

    Hip::hip->add_cpu();

    boot_lock.unlock();
}
