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

#pragma once

#include "macros.hpp"
#include "msr.hpp"

class Sgx final
{
    friend class Cpu;

    private:
        // IA32_FEATURE_CONTROL bits + lock bit
        static constexpr auto ctrl_flc { BIT (17) | BIT (0) };
        static constexpr auto ctrl_sgx { BIT (18) | BIT (0) };

        // SGX Features
        enum class Feature : unsigned
        {
            SGX1    = 0,
            SGX2    = 1,
        };

        static bool feature (Feature f) { return features & BIT (std::to_underlying (f)); }

        static uint32_t features CPULOCAL;
        static bool     writable CPULOCAL;

    public:
        class State
        {
            private:
                uint64_t lepubkeyhash[4] { 0 };

            public:
                // Obtain initial SGX FLC state programmed by firmware
                static void init()
                {
                    current.lepubkeyhash[0] = initial.lepubkeyhash[0] = Msr::read (Msr::Reg64::IA32_SGXLEPUBKEYHASH0);
                    current.lepubkeyhash[1] = initial.lepubkeyhash[1] = Msr::read (Msr::Reg64::IA32_SGXLEPUBKEYHASH1);
                    current.lepubkeyhash[2] = initial.lepubkeyhash[2] = Msr::read (Msr::Reg64::IA32_SGXLEPUBKEYHASH2);
                    current.lepubkeyhash[3] = initial.lepubkeyhash[3] = Msr::read (Msr::Reg64::IA32_SGXLEPUBKEYHASH3);
                }

                // Context-switch SGX FLC state
                void make_current() const
                {
                    if (current.lepubkeyhash[0] != lepubkeyhash[0]) [[unlikely]] Msr::write (Msr::Reg64::IA32_SGXLEPUBKEYHASH0, current.lepubkeyhash[0] = lepubkeyhash[0]);
                    if (current.lepubkeyhash[1] != lepubkeyhash[1]) [[unlikely]] Msr::write (Msr::Reg64::IA32_SGXLEPUBKEYHASH1, current.lepubkeyhash[1] = lepubkeyhash[1]);
                    if (current.lepubkeyhash[2] != lepubkeyhash[2]) [[unlikely]] Msr::write (Msr::Reg64::IA32_SGXLEPUBKEYHASH2, current.lepubkeyhash[2] = lepubkeyhash[2]);
                    if (current.lepubkeyhash[3] != lepubkeyhash[3]) [[unlikely]] Msr::write (Msr::Reg64::IA32_SGXLEPUBKEYHASH3, current.lepubkeyhash[3] = lepubkeyhash[3]);
                }

                void read (uint64_t (&v)[4]) const
                {
                    for (unsigned i { 0 }; i < 4; i++)
                        v[i] = lepubkeyhash[i];
                }

                void write (uint64_t const (&v)[4])
                {
                    // Abort if IA32_SGXLEPUBKEYHASH is not writable to avoid WRMSR faults in make_current
                    if (!writable) [[unlikely]]
                        return;

                    for (unsigned i { 0 }; i < 4; i++)
                        lepubkeyhash[i] = v[i];
                }
        };

        static State initial CPULOCAL;
        static State current CPULOCAL;

        static void init();
};
