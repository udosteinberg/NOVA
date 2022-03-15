/*
 * Control Registers (CR) / Extended Control Registers (XCR)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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
#include "types.hpp"

class Cr
{
    public:
        #define DEFINE_CR(X)                                    \
                                                                \
        ALWAYS_INLINE                                           \
        static inline auto get_cr##X()                          \
        {                                                       \
            uintptr_t val;                                      \
            asm volatile ("mov %%cr" #X ", %0" : "=r" (val));   \
            return val;                                         \
        }                                                       \
                                                                \
        ALWAYS_INLINE                                           \
        static inline void set_cr##X (uintptr_t val)            \
        {                                                       \
            asm volatile ("mov %0, %%cr" #X : : "r" (val));     \
        }

        DEFINE_CR (0);
        DEFINE_CR (2);
        DEFINE_CR (4);

        ALWAYS_INLINE
        static inline auto get_xcr (unsigned xcr)
        {
            uint32 hi, lo;
            asm volatile ("xgetbv" : "=d" (hi), "=a" (lo) : "c" (xcr));
            return static_cast<uint64>(hi) << 32 | lo;
        }

        ALWAYS_INLINE
        static inline void set_xcr (unsigned xcr, uint64 val)
        {
            asm volatile ("xsetbv" : : "d" (static_cast<uint32>(val >> 32)), "a" (static_cast<uint32>(val)), "c" (xcr));
        }
};
