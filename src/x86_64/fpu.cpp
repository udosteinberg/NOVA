/*
 * Floating Point Unit (FPU)
 * Streaming SIMD Extensions (SSE)
 * Advanced Vector Extensions (AVX)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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
#include "ec.hpp"
#include "fpu.hpp"
#include "stdio.hpp"
#include "util.hpp"

Fpu::State_xsv Fpu::hst_xsv;

void Fpu::init()
{
    if (!Cpu::feature (Cpu::Feature::XSAVE))
        return;

    // Enable supervisor state components in IA32_XSS
    if (compact)
        Msr::write (Msr::Register::IA32_XSS, hst_xsv.xss);

    // Enable user state components in XCR0
    Cr::set_xcr (0, hst_xsv.xcr);

    if (!Acpi::resume) {

        // Determine context size for all enabled state components
        uint32_t size, x;
        Cpu::cpuid (0xd, compact, x, size, x, x);
        Fpu::size = max (Fpu::size, static_cast<size_t>(size));

        trace (TRACE_FPU, "FPU%c: State:%#lx Size:%u", compact ? 'C' : 'S', hst_xsv.xcr | hst_xsv.xss, size);
    }
}

void Fpu::fini()
{
    Ec::switch_fpu (nullptr);
}
