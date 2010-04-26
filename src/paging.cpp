/*
 * IA32 Paging Support
 *
 * Author: Udo Steinberg <udo@hypervisor.org>
 * TU Dresden, Operating Systems Group
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

#include "cpu.h"
#include "paging.h"
#include "x86.h"

void Paging::enable()
{
    extern char _master_p;
    asm volatile ("mov %0, %%cr3" : : "r" (&_master_p));

    set_cr4 (Cpu::CR4_OSXMMEXCPT | Cpu::CR4_OSFXSR | Cpu::CR4_PGE | Cpu::CR4_PSE | Cpu::CR4_DE);
    set_cr0 (Cpu::CR0_PG | Cpu::CR0_WP | Cpu::CR0_NE | Cpu::CR0_TS | Cpu::CR0_MP | Cpu::CR0_PE);
}
