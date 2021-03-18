/*
 * Floating Point Unit (FPU)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#pragma once

#include "arch.hpp"
#include "cpu.hpp"
#include "hazards.hpp"
#include "lowlevel.hpp"
#include "slab.hpp"

class Fpu
{
    private:
        uint16  fcw             { 0x37f };  // x87 FPU Control Word
        uint16  fsw             { 0 };      // x87 FPU Status Word
        uint8   ftw             { 0xff };   // x87 FPU Tag Word
        uint8   reserved        { 0 };
        uint16  fop             { 0 };      // x87 FPU Opcode
        uint64  fip             { 0 };      // x87 FPU Instruction Pointer Offset
        uint64  fdp             { 0 };      // x87 FPU Instruction Data Pointer Offset
        uint32  mxcsr           { 0x1f80 }; // SIMD Control/Status Register
        uint32  mxcsr_mask      { 0 };      // SIMD Control/Status Register Mask Bits
        uint64  mmx[8][2]       {{0}};      //  8  80bit MMX registers
        uint64  xmm[16][2]      {{0}};      // 16 128bit XMM registers
        uint64  unused[6][2]    {{0}};      //  6 128bit reserved

        static Slab_cache cache;

    public:
        ALWAYS_INLINE
        inline void load() const { asm volatile ("fxrstor %0" : : "m" (*this)); }

        ALWAYS_INLINE
        inline void save() { asm volatile ("fxsave %0" : "=m" (*this)); }

        ALWAYS_INLINE
        static inline void disable()
        {
            set_cr0 (get_cr0() | CR0_TS);

            Cpu::hazard &= ~HZD_FPU;
        }

        ALWAYS_INLINE
        static inline void enable()
        {
            asm volatile ("clts");

            Cpu::hazard |= HZD_FPU;
        }

        NODISCARD ALWAYS_INLINE
        static inline void *operator new (size_t) noexcept
        {
            static_assert (sizeof (Fpu) == 512);
            return cache.alloc();
        }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                cache.free (ptr);
        }
};
