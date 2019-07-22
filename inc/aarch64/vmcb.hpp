/*
 * Virtual Machine Control Block (VMCB)
 *
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

#include "buddy.hpp"
#include "interrupt.hpp"
#include "types.hpp"

class Vmcb final
{
    public:
        struct {                                    // EL1 Registers
            uint64_t    afsr0       {};             // Auxiliary Fault Status Register 0
            uint64_t    afsr1       {};             // Auxiliary Fault Status Register 1
            uint64_t    amair       {};             // Auxiliary Memory Attribute Indirection Register
            uint64_t    cntkctl     {};             // Counter-Timer Kernel Control Register
            uint64_t    contextidr  {};             // Context ID Register
            uint64_t    cpacr       {};             // Architectural Feature Access Control Register
            uint64_t    csselr      {};             // Cache Size Selection Register
            uint64_t    elr         {};             // Exception Link Register
            uint64_t    esr         {};             // Exception Syndrome Register
            uint64_t    far         {};             // Fault Address Register
            uint64_t    mair        {};             // Memory Attribute Indirection Register
            uint64_t    mdscr       {};             // Monitor Debug System Control Register
            uint64_t    par         {};             // Physical Address Register
            uint64_t    sctlr       {};             // System Control Register
            uint64_t    sp          {};             // Stack Pointer
            uint64_t    spsr        {};             // Saved Program Status Register
            uint64_t    tcr         {};             // Translation Control Register
            uint64_t    tpidr       {};             // Software Thread ID Register
            uint64_t    ttbr0       {};             // Translation Table Base Register 0
            uint64_t    ttbr1       {};             // Translation Table Base Register 1
            uint64_t    vbar        {};             // Vector Base Address Register
        } el1;

        struct {                                    // EL2 Registers
            uint64_t    hcr         {};             // Hypervisor Configuration Register
            uint64_t    hcrx        {};             // Extended Hypervisor Configuration Register
            uint64_t    hpfar       {};             // Hypervisor IPA Fault Address Register
            uint64_t    vdisr       {};             // Virtual Deferred Interrupt Status Register
            uint64_t    vmpidr      {};             // Virtualization Multiprocessor ID Register
            uint64_t    vpidr       {};             // Virtualization Processor ID Register
        } el2;

        struct {                                    // AArch32 Registers
            uint32_t    dacr        {};             // Domain Access Control Register
            uint32_t    fpexc       {};             // Floating-Point Exception Control Register
            uint32_t    hstr        {};             // Hypervisor System Trap Register
            uint32_t    ifsr        {};             // Instruction Fault Status Register
            uint32_t    spsr_abt    {};             // Saved Program Status Register (Abort Mode)
            uint32_t    spsr_fiq    {};             // Saved Program Status Register (FIQ Mode)
            uint32_t    spsr_irq    {};             // Saved Program Status Register (IRQ Mode)
            uint32_t    spsr_und    {};             // Saved Program Status Register (Undefined Mode)
        } a32;

        struct {                                    // Timer Registers
            uint64_t    cntvoff     {};             // Virtual Offset Register
            uint64_t    cntv_cval   {};             // Virtual Timer CompareValue Register
            uint64_t    cntv_ctl    {};             // Virtual Timer Control Register
            bool        cntv_act    {};             // Virtual Timer PPI Active State
        } tmr;

        struct {                                    // GIC Registers
            uint64_t    lr[16]      {};             // List Registers
            uint32_t    ap0r[4]     {};             // Active Priorities Group 0 Registers
            uint32_t    ap1r[4]     {};             // Active Priorities Group 1 Registers
            uint32_t    elrsr       {};             // Empty List Register Status Register
            uint32_t    vmcr        {};             // Virtual Machine Control Register
            uint32_t    hcr         { BIT (0) };    // Hypervisor Control Register
        } gic;

        static Vmcb const *current CPULOCAL;

        ALWAYS_INLINE
        inline void save_tmr()
        {
            tmr.cntv_act = Interrupt::tmr_act_get();
        }

        ALWAYS_INLINE
        inline void load_tmr() const
        {
            Interrupt::tmr_act_set (tmr.cntv_act);
        }

        static void init();
        static void load_hst();

        void load_gst() const;
        void save_gst();

        /*
         * Allocate VMCB
         *
         * @return      Pointer to the VMCB (allocation success) or nullptr (allocation failure)
         */
        [[nodiscard]] static void *operator new (size_t) noexcept
        {
            return Buddy::alloc (0);
        }

        /*
         * Deallocate VMCB
         *
         * @param ptr   Pointer to the VMCB (or nullptr)
         */
        static void operator delete (void *ptr)
        {
            Buddy::free (ptr);
        }
};

// Sanity checks
static_assert (sizeof (Vmcb) <= PAGE_SIZE (0));
