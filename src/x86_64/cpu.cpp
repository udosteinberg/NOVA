/*
 * Central Processing Unit (CPU)
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
#include "msr.hpp"
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

void Cpu::check_features (uint32 (&name)[12], unsigned &package, unsigned &core, unsigned &thread)
{
    unsigned tpp = 1, cpp = 1;

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
            cpuid (0x7, 0, eax, features[3], features[4], features[5]);
            FALLTHROUGH;
        case 0x6:
            cpuid (0x6, features[2], ebx, ecx, edx);
            FALLTHROUGH;
        case 0x4 ... 0x5:
            cpuid (0x4, 0, eax, ebx, ecx, edx);
            cpp = (eax >> 26 & 0x3f) + 1;
            FALLTHROUGH;
        case 0x1 ... 0x3:
            cpuid (0x1, eax, ebx, features[1], features[0]);
            family   = (eax >> 8 & 0xf) + (eax >> 20 & 0xff);
            model    = (eax >> 4 & 0xf) + (eax >> 12 & 0xf0);
            stepping =  eax & 0xf;
            topology =  ebx >> 24;
            tpp      =  ebx >> 16 & 0xff;
            Cache::dcache_line_size = 8 * (ebx >> 8 & 0xff);
            Cache::icache_line_size = 8 * (ebx >> 8 & 0xff);
    }

    patch = static_cast<unsigned>(Msr::read (Msr::IA32_BIOS_SIGN_ID) >> 32);

    cpuid (0x80000000, eax, ebx, ecx, edx);

    if (eax & 0x80000000) {
        switch (static_cast<uint8>(eax)) {
            default:
                cpuid (0x8000000a, Vmcb::svm_version, ebx, ecx, Vmcb::svm_feature);
                FALLTHROUGH;
            case 0x4 ... 0x9:
                cpuid (0x80000004, name[8], name[9], name[10], name[11]);
                FALLTHROUGH;
            case 0x3:
                cpuid (0x80000003, name[4], name[5], name[6], name[7]);
                FALLTHROUGH;
            case 0x2:
                cpuid (0x80000002, name[0], name[1], name[2], name[3]);
                FALLTHROUGH;
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

    if (EXPECT_FALSE (Cmdline::nopcid))
        defeature (Feature::PCID);
}

void Cpu::setup_msr()
{
    if (EXPECT_TRUE (feature (Feature::ACPI)))
        Msr::write (Msr::IA32_THERM_INTERRUPT, 0x10);

    if (EXPECT_TRUE (feature (Feature::SEP)))
        Msr::write (Msr::IA32_SYSENTER_CS, 0);

    if (EXPECT_TRUE (feature (Feature::LM))) {
        Msr::write (Msr::IA32_STAR,  static_cast<uint64>(SEL_USER_CODE) << 48 | static_cast<uint64>(SEL_KERN_CODE) << 32);
        Msr::write (Msr::IA32_LSTAR, reinterpret_cast<uint64>(&entry_sys));
        Msr::write (Msr::IA32_FMASK, RFL_VIP | RFL_VIF | RFL_AC | RFL_VM | RFL_RF | RFL_NT | RFL_IOPL | RFL_DF | RFL_IF | RFL_TF);
    }

    if (EXPECT_TRUE (feature (Feature::RDT_M)))
        trace (TRACE_CPU, "QoS monitoring supported");

    if (EXPECT_TRUE (feature (Feature::RDT_A)))
        trace (TRACE_CPU, "QoS allocation supported");

    // Disable C1E on AMD Rev.F and beyond because it stops LAPIC clock
    if (vendor == Vendor::AMD)
        if (family > 0xf || (family == 0xf && model >= 0x40))
            Msr::write (Msr::AMD_IPMR, Msr::read (Msr::AMD_IPMR) & ~(3UL << 27));
}

void Cpu::init()
{
    uint32 name[12] = { 0 };
    unsigned package, core, thread;

    if (!Acpi::resume) {

        for (void (**func)() = &CTORS_L; func != &CTORS_C; (*func++)()) ;

        Gdt::build();
        Tss::build();
    }

    // Initialize exception handling
    Gdt::load();
    Idt::load();
    Tss::load();

    check_features (name, package, core, thread);

    Lapic::init();

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
