/*
 * Safer Mode Extensions (SMX)
 *
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

#pragma once

#include "compiler.hpp"
#include "macros.hpp"
#include "types.hpp"

class Smx final
{
    private:
        enum Leaf : unsigned
        {
            CAPABILITIES    = 0,
            ENTERACCS       = 2,
            EXITAC          = 3,
            SENTER          = 4,
            SEXIT           = 5,
            PARAMETERS      = 6,
            SMCTRL          = 7,
            WAKEUP          = 8,
        };

    public:
        static constexpr auto required { BIT (WAKEUP) | BIT (SMCTRL) | BIT (SEXIT) | BIT (SENTER) | BIT (CAPABILITIES) };

        /*
         * Return SMX Capabilities
         */
        static auto capabilities()
        {
            uint32_t val;
            asm volatile ("getsec" : "=a" (val) : "a" (Leaf::CAPABILITIES), "b" (0));
            return val;
        }

        /*
         * Enter Measured Environment
         *
         * @param base  SINIT ACM physical base address
         * @param size  SINIT ACM size (bytes)
         */
        [[noreturn]] static void senter (uint32_t base, uint32_t size)
        {
            asm volatile ("getsec" : : "a" (Leaf::SENTER), "b" (base), "c" (size), "d" (0));
            UNREACHED;
        }

        /*
         * Exit Measured Environment
         */
        static void sexit() { asm volatile ("getsec" : : "a" (Leaf::SEXIT)); }

        /*
         * Return SMX Parameters
         *
         * @param index Parameter Index
         * @param eax   Result
         * @param ebx   Result
         * @param ecx   Result
         * @return      Parameter Type
         */
        static auto parameters (unsigned index, uint32_t &eax, uint32_t &ebx, uint32_t &ecx)
        {
            asm volatile ("getsec" : "=a" (eax), "=b" (ebx), "=c" (ecx) : "a" (Leaf::PARAMETERS), "b" (index));
            return eax & BIT_RANGE (4, 0);
        }

        /*
         * SMX Mode Control
         */
        static void smctrl() { asm volatile ("getsec" : : "a" (Leaf::SMCTRL), "b" (0)); }

        /*
         * Wake Up RLPs in Measured Environment
         */
        static void wakeup() { asm volatile ("getsec" : : "a" (Leaf::WAKEUP)); }

        /*
         * MLE Join Structure
         */
        static inline struct { uint32_t gdt_limit, gdt_base, sel, eip; } mle_join asm ("mle_join") { 0, 0, 0, 0 };
};
