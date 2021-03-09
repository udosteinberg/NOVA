/*
 * Interrupt Descriptor Table (IDT)
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

#include "arch.hpp"
#include "extern.hpp"
#include "idt.hpp"
#include "selectors.hpp"

void Idt::build()
{
    uintptr_t *ptr { handlers };

    for (unsigned vector { 0 }; vector < sizeof (idt) / sizeof (*idt); vector++, ptr++)
        idt[vector] = Descriptor_idt { *ptr & IDT_USER ? 3U : 0U, *ptr & IDT_IST1 ? 1U : 0U, SEL_KERN_CODE, *ptr & ~IDT_MASK };
}
