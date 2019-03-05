/*
 * Secure Monitor Calls (SMC)
 *
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "types.hpp"

class Smc
{
    private:
        static mword smc (mword p0, mword &p1, mword &p2, mword &p3, mword p4, mword p5, mword p6, mword p7)
        {
            register mword r0 asm ("r0") = p0, r1 asm ("r1") = p1, r2 asm ("r2") = p2, r3 asm ("r3") = p3;
            register mword r4 asm ("r4") = p4, r5 asm ("r5") = p5, r6 asm ("r6") = p6, r7 asm ("r7") = p7;

            asm volatile ("smc #0"
                          : "+r" (r0), "+r" (r1), "+r" (r2), "+r" (r3), "+r" (r4), "+r" (r5), "+r" (r6), "+r" (r7)
                          :
                          : "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "r16", "r17", "memory");

            p1 = r1; p2 = r2; p3 = r3;

            return r0;
        }

    public:
        enum class Service
        {
            ARM = 0x00,
            CPU = 0x01,
            SIP = 0x02,
            OEM = 0x03,
            SEC = 0x04,
            HYP = 0x05,
            VEN = 0x06,
            TOS = 0x3f,
        };

        enum class Function
        {
            CNT = 0xff00,
            UID = 0xff01,
            REV = 0xff03,
        };

        static Service  srv (uint32 p) { return Service  (p >> 24 & 0x3f); }
        static Function fun (uint32 p) { return Function (p & 0xffff); }

        template <typename T>
        static mword call (Service s, Function f, mword &p1, mword &p2, mword &p3, mword p4 = 0, mword p5 = 0, mword p6 = 0, mword p7 = 0)
        {
            return smc (1U << 31 | (sizeof (T) == sizeof (uint64)) << 30 | uint32 (s) << 24 | uint16 (f), p1, p2, p3, p4, p5, p6, p7);
        }

        static void init();
};
