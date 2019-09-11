/*
 * Floating Point Unit (FPU)
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

#include "compiler.hpp"
#include "cpu.hpp"
#include "hazards.hpp"
#include "slab.hpp"
#include "types.hpp"

class Fpu
{
    private:
        uint64  v[32][2]        {{0}};      // 32 128bit SIMD and FP Registers
        uint64  fpcr            { 0 };      // Floating-Point Control Register
        uint64  fpsr            { 0 };      // Floating-Point Status Register

        static Slab_cache       cache;

    public:
        void load() const
        {
            uint64 dummy;

            asm volatile ("ldp  q0,  q1,  [%0, #32* 0]  ;"
                          "ldp  q2,  q3,  [%0, #32* 1]  ;"
                          "ldp  q4,  q5,  [%0, #32* 2]  ;"
                          "ldp  q6,  q7,  [%0, #32* 3]  ;"
                          "ldp  q8,  q9,  [%0, #32* 4]  ;"
                          "ldp  q10, q11, [%0, #32* 5]  ;"
                          "ldp  q12, q13, [%0, #32* 6]  ;"
                          "ldp  q14, q15, [%0, #32* 7]  ;"
                          "ldp  q16, q17, [%0, #32* 8]  ;"
                          "ldp  q18, q19, [%0, #32* 9]  ;"
                          "ldp  q20, q21, [%0, #32*10]  ;"
                          "ldp  q22, q23, [%0, #32*11]  ;"
                          "ldp  q24, q25, [%0, #32*12]  ;"
                          "ldp  q26, q27, [%0, #32*13]  ;"
                          "ldp  q28, q29, [%0, #32*14]  ;"
                          "ldp  q30, q31, [%0, #32*15]! ;"  // clobbers %0
                          "ldp   %1,  %2, [%0, #32* 1]  ;"  // clobbers %1, %2
                          "msr  fpcr, %1                ;"
                          "msr  fpsr, %2                ;"
                          : "=&r" (dummy), "=&r" (dummy), "=&r" (dummy) : "m" (*this), "0" (v));
        }

        void save()
        {
            uint64 dummy;

            asm volatile ("mrs  %1,  fpcr               ;"  // clobbers %1
                          "mrs  %2,  fpsr               ;"  // clobbers %2
                          "stp  q0,  q1,  [%0, #32* 0]  ;"
                          "stp  q2,  q3,  [%0, #32* 1]  ;"
                          "stp  q4,  q5,  [%0, #32* 2]  ;"
                          "stp  q6,  q7,  [%0, #32* 3]  ;"
                          "stp  q8,  q9,  [%0, #32* 4]  ;"
                          "stp  q10, q11, [%0, #32* 5]  ;"
                          "stp  q12, q13, [%0, #32* 6]  ;"
                          "stp  q14, q15, [%0, #32* 7]  ;"
                          "stp  q16, q17, [%0, #32* 8]  ;"
                          "stp  q18, q19, [%0, #32* 9]  ;"
                          "stp  q20, q21, [%0, #32*10]  ;"
                          "stp  q22, q23, [%0, #32*11]  ;"
                          "stp  q24, q25, [%0, #32*12]  ;"
                          "stp  q26, q27, [%0, #32*13]  ;"
                          "stp  q28, q29, [%0, #32*14]  ;"
                          "stp  q30, q31, [%0, #32*15]! ;"  // clobbers %0
                          "stp   %1,  %2, [%0, #32* 1]  ;"
                          : "=&r" (dummy), "=&r" (dummy), "=&r" (dummy), "=m" (*this) : "0" (v));
        }

        ALWAYS_INLINE
        static inline void disable()
        {
            asm volatile ("msr cptr_el2, %0" : : "r" (Cpu::cptr | CPTR_TFP));

            Cpu::hazard &= ~HZD_FPU;
        }

        ALWAYS_INLINE
        static inline void enable()
        {
            asm volatile ("msr cptr_el2, %0" : : "r" (Cpu::cptr));

            Cpu::hazard |= HZD_FPU;
        }

        static inline void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }

        static inline void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                cache.free (ptr);
        }
};
