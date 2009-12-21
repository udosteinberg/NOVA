/*
 * i8259A Programmable Interrupt Controller (PIC)
 *
 * Copyright (C) 2006-2009, Udo Steinberg <udo@hypervisor.org>
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

#include "pic.h"
#include "stdio.h"

/*
 * Platform Master & Slave PIC
 */
Pic Pic::master  (ID_MASTER, PORT_MASTER, 0xfb);
Pic Pic::slave   (ID_SLAVE,  PORT_SLAVE,  0xff);

/*
 * PIC Initialization
 */
void Pic::init (unsigned vector)
{
    // Initialize
    out (ICW1, ICW1_SELECT | ICW1_ICW4);
    out (ICW2, vector);
    out (ICW3, _id);
    out (ICW4, ICW4_8086);
    out (OCW1, _mask);

    trace (TRACE_INTERRUPT, "PIC: VEC:%#x IMR:%#x from IRR:%#x ISR:%#x IMR:%#x",
           vector, _mask, irr(), isr(), imr());
}

void Pic::eoi()
{
    out (OCW2, OCW2_SELECT | OCW2_EOI);
}

void Pic::pin_mask (unsigned pin)
{
    out (OCW1, _mask |= (1u << pin));
}

void Pic::pin_unmask (unsigned pin)
{
    out (OCW1, _mask &= ~(1u << pin));
}
