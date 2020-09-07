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
#include "fpu.hpp"
#include "gdt.hpp"
#include "hazards.hpp"
#include "idt.hpp"
#include "lapic.hpp"
#include "lowlevel.hpp"
#include "mca.hpp"
#include "pd_kern.hpp"
#include "stdio.hpp"
#include "svm.hpp"
#include "tss.hpp"
#include "vmx.hpp"

unsigned    Cpu::id;
unsigned    Cpu::hazard;

Cpu::Vendor Cpu::vendor;
unsigned    Cpu::platform;
unsigned    Cpu::family;
unsigned    Cpu::model;
unsigned    Cpu::stepping;
unsigned    Cpu::patch;

uint32      Cpu::features[8];
uint32      Cpu::topology;
bool        Cpu::bsp;

void Cpu::enumerate_clocks (uint32 &clk, uint32 &rat, scaleable_bus const *freq, unsigned i)
{
    clk = freq[i].d ? 100'000'000 * freq[i].m / freq[i].d : 0;
    rat = Msr::read (Msr::PLATFORM_INFO) >> 8 & BIT_RANGE (7, 0);
}

void Cpu::enumerate_clocks (uint32 &clk, uint32 &rat)
{
    // Intel P-core >= CNL, E-core >= GLM+: CPUID[0x15] reports both clk and rat
    if (clk && rat)
        return;

    if (vendor != Vendor::INTEL || family != 0x6)
        return;

    // Use model-specific knowledge
    switch (model) {

        // P-core >= SKL: CPUID[0x15] reports rat only
        case 0xa6:              // CML-U62
        case 0xa5:              // CML-H, CML-S
        case 0x9e:              // KBL-H, KBL-S, KBL-G, KBL-X, CFL-H, CFL-S, Xeon E3v6
        case 0x8e:              // KBL-Y, KBL-U, KBL-R, AML-Y, WHL-U, CFL-U, CML-U42
        case 0x5e:              // SKL-H, SKL-S, Xeon E3v5
        case 0x4e:              // SKL-Y, SKL-U
            clk = 24'000'000;   // 24 MHz
            return;

        // P-core <= BDW: CPUID[0x15] unavailable
        case 0x56:              // BDW-DE
        case 0x4f:              // BDW-E, BDW-EP, BDW-EX
        case 0x3f:              // HSW-E, HSW-EP, HSW-EX
        case 0x3e:              // IVB-E, IVB-EP, IVB-EX
        case 0x2d:              // SNB-E, SNB-EP, SNB-EX
        case 0x47:              // BDW-H, Xeon E3v4
        case 0x3d:              // BDW-Y, BDW-U
        case 0x46:              // HSW-H, HSW-R
        case 0x45:              // HSW-Y, HSW-U
        case 0x3c:              // HSW-M, Xeon E3v3
        case 0x3a:              // IVB, Xeon E3v2, Gladden
        case 0x2a:              // SNB, Xeon E3v1
            return enumerate_clocks (clk, rat, freq_core, 5);   // 100.00 MHz

        case 0x2f:              // WSM-EX (Eagleton)
        case 0x2c:              // WSM-EP (Gulftown)
        case 0x25:              // WSM (Clarkdale, Arrandale)
        case 0x2e:              // NHM-EX (Beckton)
        case 0x1a:              // NHM-EP (Gainestown, Bloomfield)
        case 0x1f:              // NHM (Havendale, Auburndale)
        case 0x1e:              // NHM (Lynnfield, Clarksfield, Jasper Forest)
            return enumerate_clocks (clk, rat, freq_core, 1);   // 133.33 MHz

        case 0x1d:              // Dunnington
        case 0x17:              // Penryn, Wolfdale, Yorkfield, Harpertown
        case 0x0f:              // Merom, Conroe, Kentsfield, Clovertown, Woodcrest, Tigerton
            return enumerate_clocks (clk, rat, freq_core, Msr::read (Msr::FSB_FREQ) & BIT_RANGE (2, 0));

        // E-core >= GLM: CPUID[0x15] reports rat only
        case 0x5f:              // GLM (Harrisonville: Denverton)
            clk = 25'000'000;   // 25 MHz
            return;

        case 0x5c:              // GLM (Willow Trail: Broxton, Apollo Lake)
            clk = 19'200'000;   // 19.2 MHz
            return;

        // E-core <= AMT: CPUID[0x15] unavailable
        case 0x4c:              // AMT (Cherry Trail: Cherryview, Braswell)
            return enumerate_clocks (clk, rat, freq_atom, Msr::read (Msr::FSB_FREQ) & BIT_RANGE (3, 0));

        case 0x5d:              // SLM (Slayton: Sofia 3G)
        case 0x5a:              // SLM (Moorefield: Anniedale)
        case 0x4a:              // SLM (Merrifield: Tangier)
        case 0x37:              // SLM (Bay Trail: Valleyview)
            return enumerate_clocks (clk, rat, freq_atom, Msr::read (Msr::FSB_FREQ) & BIT_RANGE (2, 0));

        case 0x75:              // AMT (Lightning Mountain)
        case 0x6e:              // AMT (Cougar Mountain)
        case 0x65:              // AMT (XGold XMM7272)
        case 0x4d:              // SLM (Edisonville: Avoton, Rangeley)
            return;
    }
}

void Cpu::enumerate_features (uint32 &clk, uint32 &rat, uint32 (&name)[12], unsigned &package, unsigned &core, unsigned &thread)
{
    unsigned tpp { 1 }, cpp { 1 };

    uint32 eax, ebx, ecx, edx;

    cpuid (0, eax, ebx, ecx, edx);

    size_t v;
    for (v = sizeof (vendor_string) / sizeof (*vendor_string); --v;)
        if (*reinterpret_cast<uint32 const *>(vendor_string[v] + 0) == ebx &&
            *reinterpret_cast<uint32 const *>(vendor_string[v] + 4) == edx &&
            *reinterpret_cast<uint32 const *>(vendor_string[v] + 8) == ecx)
            break;

    vendor = Vendor (v);

    if (vendor == Vendor::INTEL) {
        Msr::write (Msr::IA32_BIOS_SIGN_ID, 0);
        platform = static_cast<unsigned>(Msr::read (Msr::IA32_PLATFORM_ID) >> 50) & 7;
    }

    switch (static_cast<uint8>(eax)) {
        default:
            cpuid (0x15, eax, ebx, clk, edx);
            rat = eax ? ebx / eax : 0;
            [[fallthrough]];
        case 0x7 ... 0x14:
            cpuid (0x7, 0, eax, features[3], features[4], features[5]);
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
            topology =  ebx >> 24;
            tpp      =  ebx >> 16 & 0xff;
            Cache::init (8 * (ebx >> 8 & 0xff));
    }

    patch = static_cast<unsigned>(Msr::read (Msr::IA32_BIOS_SIGN_ID) >> 32);

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
                cpuid (0x80000001, eax, ebx, features[7], features[6]);
        }
    }

    if (feature (Feature::CMP_LEGACY))
        cpp = tpp;

    unsigned tpc = tpp / cpp;
    unsigned long t_bits = bit_scan_reverse (tpc - 1) + 1;
    unsigned long c_bits = bit_scan_reverse (cpp - 1) + 1;

    thread  = topology            & ((1u << t_bits) - 1);
    core    = topology >>  t_bits & ((1u << c_bits) - 1);
    package = topology >> (t_bits + c_bits);

    if (EXPECT_FALSE (Cmdline::nodl))
        defeature (Feature::TSC_DEADLINE);

    if (EXPECT_FALSE (Cmdline::nopcid))
        defeature (Feature::PCID);

    enumerate_clocks (clk, rat);
}

void Cpu::setup_msr()
{
    if (EXPECT_TRUE (feature (Feature::ACPI)))
        Msr::write (Msr::IA32_THERM_INTERRUPT, 0x10);

    if (EXPECT_TRUE (feature (Feature::SEP)))
        Msr::write (Msr::IA32_SYSENTER_CS, 0);

    if (EXPECT_TRUE (feature (Feature::LM))) {
        Msr::write (Msr::IA32_STAR,  msr.star);
        Msr::write (Msr::IA32_LSTAR, msr.lstar);
        Msr::write (Msr::IA32_FMASK, msr.fmask);
        Msr::write (Msr::IA32_KERNEL_GS_BASE, msr.kernel_gs_base);
    }

    if (EXPECT_TRUE (feature (Feature::RDT_M)))
        trace (TRACE_CPU, "FEAT: QoS monitoring supported");

    if (EXPECT_TRUE (feature (Feature::RDT_A)))
        trace (TRACE_CPU, "FEAT: QoS allocation supported");

    // Disable C1E on AMD Rev.F and beyond because it stops LAPIC clock
    if (vendor == Vendor::AMD)
        if (family > 0xf || (family == 0xf && model >= 0x40))
            Msr::write (Msr::AMD_IPMR, Msr::read (Msr::AMD_IPMR) & ~(3UL << 27));
}

void Cpu::init()
{
    uint32 clk { 0 }, rat { 0 }, name[12] { 0 };
    unsigned package, core, thread;

    if (!Acpi::resume) {

        for (void (**func)() = &CTORS_L; func != &CTORS_C; (*func++)()) ;

        hazard = HZD_BOOT_GST | HZD_BOOT_HST;

        Gdt::build();
        Tss::build();
    }

    // Initialize exception handling
    Gdt::load();
    Idt::load();
    Tss::load();

    enumerate_features (clk, rat, name, package, core, thread);

    Lapic::init (clk, rat);

    if (!Acpi::resume) {
        uint64 phys; unsigned o; Memattr::Cacheability ca; Memattr::Shareability sh;
        Pd_kern::nova().Space_mem::loc[id] = Hptp::current();
        Pd_kern::nova().Space_mem::loc[id].lookup (MMAP_CPU_DATA, phys, o, ca, sh);
        Hptp::master.update (MMAP_GLB_DATA + id * PAGE_SIZE, phys, 0, Paging::Permissions (Paging::G | Paging::W | Paging::R), ca, sh);
        Hptp::set_leaf_max (feature (Feature::GB_PAGES) ? 3 : 2);
    }

    setup_msr();

    set_cr4 (get_cr4() | feature (Feature::SMAP) * CR4_SMAP  |
                         feature (Feature::SMEP) * CR4_SMEP  |
                         feature (Feature::PCID) * CR4_PCIDE |
                         feature (Feature::UMIP) * CR4_UMIP);

    Vmcs::init();
    Vmcb::init();

    Mca::init();

    trace (TRACE_CPU, "CORE: %x:%x:%x %x:%x:%x:%x [%x] %.48s", package, core, thread, family, model, stepping, platform, patch, reinterpret_cast<char *>(name));

    boot_lock.unlock();
}

void Cpu::fini()
{
    hazard &= ~HZD_SLEEP;

    auto s = Acpi::get_transition();

    if (s.state() > 1) {
        Fpu::fini();
        Vmcs::fini();
    }

    Acpi::fini (s);
}
