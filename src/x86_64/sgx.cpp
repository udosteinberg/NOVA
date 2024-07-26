/*
 * Software Guard Extensions (SGX)
 *
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

#include "cpu.hpp"
#include "sgx.hpp"
#include "stdio.hpp"

Sgx::State  Sgx::current, Sgx::initial;
uint32_t    Sgx::features { 0 };
bool        Sgx::writable { false };

void Sgx::init()
{
    // Abort if SGX is not supported
    if (!Cpu::feature (Cpu::Feature::SGX)) [[unlikely]]
        return;

    auto const ctrl { Msr::read (Msr::Reg64::IA32_FEATURE_CONTROL) };

    // Abort if firmware has opted out of SGX
    if ((ctrl & ctrl_sgx) != ctrl_sgx) [[unlikely]]
        return;

    trace (TRACE_CPU, "ENCL: Features:%#x%s%s%s", features, Cpu::feature (Cpu::Feature::SGX_LC) ? " FLC" : "", feature (Feature::SGX2) ? " SGX2" : "", feature (Feature::SGX1) ? " SGX1" : "");

    // Abort if SGX1 functions are not available
    if (!feature (Feature::SGX1)) [[unlikely]]
        return;

    // Check if SGX launch control is available
    if (Cpu::feature (Cpu::Feature::SGX_LC)) [[likely]] {

        // Obtain initial state
        State::init();

        // IA32_SGXLEPUBKEYHASH is writable if firmware has enabled FLC
        writable = (ctrl & ctrl_flc) == ctrl_flc;
    }
}
