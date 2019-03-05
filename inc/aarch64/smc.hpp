/*
 * Secure Monitor Calls (SMC)
 *
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

#pragma once

#include "compiler.hpp"
#include "macros.hpp"
#include "std.hpp"
#include "types.hpp"

class Smc
{
    private:
        static inline bool permitted (uint32 fid)
        {
            unsigned s { fid >> 24 & BIT_RANGE (5, 0) };

            switch (static_cast<Service>(s)) {
                case Service::SEC:
                    return (BIT (19) | BIT (5)) & BIT (static_cast<uint16>(fid) >> 4);
                default:
                    return (BIT64_RANGE (63, 48) | BIT64 (2)) & BIT64 (s);
            }
        }

    protected:
        enum class Call : uint32
        {
            SMC32   = 0,    // SMC32 Calling Convention
            SMC64   = 1,    // SMC64 Calling Convention
        };

        enum class Service : uint32
        {
            ARM     = 0,    // ARM Architecture Calls
            CPU     = 1,    // CPU Service Calls
            SIP     = 2,    // SiP Service Calls
            OEM     = 3,    // OEM Service Calls
            SEC     = 4,    // Standard Secure Service Calls
            HYP     = 5,    // Standard Hypervisor Service Calls
            VEN     = 6,    // Vendor Specific Hypervisor Service Calls
        };

        ALWAYS_INLINE
        static inline auto fid (Call c, Service s, uint16 f)
        {
            return BIT (31) | std::to_underlying (c) << 30 | std::to_underlying (s) << 24 | f;
        }

        ALWAYS_INLINE
        static inline auto call (Call c, Service s, uint16 f, uintptr_t &p1, uintptr_t &p2, uintptr_t &p3)
        {
            register auto r0 asm ("r0") { fid (c, s, f) };
            register auto r1 asm ("r1") { p1 };
            register auto r2 asm ("r2") { p2 };
            register auto r3 asm ("r3") { p3 };

            asm volatile ("smc #0" : "+r" (r0), "+r" (r1), "+r" (r2), "+r" (r3) : : "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "r16", "r17");

            p1 = r1; p2 = r2; p3 = r3;

            return static_cast<int32_t>(r0);
        }

    public:
        ALWAYS_INLINE
        static inline auto proxy (uintptr_t (&r)[31])
        {
            if (!permitted (static_cast<uint32>(r[0])))
                return false;

            asm volatile ("ldp  x0,  x1, [%1, #16*0];"
                          "ldp  x2,  x3, [%1, #16*1];"
                          "ldp  x4,  x5, [%1, #16*2];"
                          "ldp  x6,  x7, [%1, #16*3];"
                          "ldp  x8,  x9, [%1, #16*4];"
                          "ldp x10, x11, [%1, #16*5];"
                          "ldp x12, x13, [%1, #16*6];"
                          "ldp x14, x15, [%1, #16*7];"
                          "ldp x16, x17, [%1, #16*8];"
                          "smc #0                   ;"
                          "stp  x0,  x1, [%1, #16*0];"
                          "stp  x2,  x3, [%1, #16*1];"
                          "stp  x4,  x5, [%1, #16*2];"
                          "stp  x6,  x7, [%1, #16*3];"
                          "stp  x8,  x9, [%1, #16*4];"
                          "stp x10, x11, [%1, #16*5];"
                          "stp x12, x13, [%1, #16*6];"
                          "stp x14, x15, [%1, #16*7];"
                          "stp x16, x17, [%1, #16*8];"
                          : "+m" (r) : "r" (r) : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "r16", "r17");

            return true;
        }
};
