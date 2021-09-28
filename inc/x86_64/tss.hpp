/*
 * Task State Segment (TSS)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "compiler.hpp"
#include "gdt.hpp"

class Tss final
{
    public:
        Unaligned_le<uint32_t>  res0;               // 0
        Unaligned_le<uint64_t>  rsp[3];             // 4
        Unaligned_le<uint64_t>  ist[8];             // 28
        Unaligned_le<uint64_t>  res1;               // 92
        Unaligned_le<uint16_t>  res2;               // 100
        Unaligned_le<uint16_t>  iobm;               // 102

        static Tss run asm ("tss_run")  CPULOCAL;

        static void build();

        static void load()
        {
            Gdt::unbusy_tss();
            asm volatile ("ltr %w0" : : "rm" (SEL_TSS_RUN));
        }
};

static_assert (__is_standard_layout (Tss) && alignof (Tss) == 1 && sizeof (Tss) == 104);
