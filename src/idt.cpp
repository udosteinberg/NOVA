/*
 * Interrupt Descriptor Table (IDT)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "extern.h"
#include "idt.h"
#include "selectors.h"

ALIGNED(8) Idt Idt::idt[VEC_MAX];

void Idt::build()
{
    mword *ptr = handlers;

    for (unsigned vector = 0; vector < VEC_MAX; vector++, ptr++)
        if (*ptr)
            idt[vector].set (SYS_INTR_GATE, *ptr & 3, SEL_KERN_CODE, *ptr & ~3);
        else
            idt[vector].set (SYS_TASK_GATE, 0, SEL_TSS_DBF, 0);
}
