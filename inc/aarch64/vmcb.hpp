/*
 * Virtual Machine Control Block (VMCB)
 *
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

#include "buddy.hpp"
#include "interrupt.hpp"
#include "types.hpp"

class Vmcb final
{
    public:
        struct {                                // EL1 Registers
            uint64  afsr0       { 0 };          // Auxiliary Fault Status Register 0
            uint64  afsr1       { 0 };          // Auxiliary Fault Status Register 1
            uint64  amair       { 0 };          // Auxiliary Memory Attribute Indirection Register
            uint64  contextidr  { 0 };          // Context ID Register
            uint64  cpacr       { 0 };          // Architectural Feature Access Control Register
            uint64  csselr      { 0 };          // Cache Size Selection Register
            uint64  elr         { 0 };          // Exception Link Register
            uint64  esr         { 0 };          // Exception Syndrome Register
            uint64  far         { 0 };          // Fault Address Register
            uint64  mair        { 0 };          // Memory Attribute Indirection Register
            uint64  mdscr       { 0 };          // Monitor Debug System Control Register
            uint64  par         { 0 };          // Physical Address Register
            uint64  sctlr       { 0 };          // System Control Register
            uint64  sp          { 0 };          // Stack Pointer
            uint64  spsr        { 0 };          // Saved Program Status Register
            uint64  tcr         { 0 };          // Translation Control Register
            uint64  tpidr       { 0 };          // Software Thread ID Register
            uint64  ttbr0       { 0 };          // Translation Table Base Register 0
            uint64  ttbr1       { 0 };          // Translation Table Base Register 1
            uint64  vbar        { 0 };          // Vector Base Address Register
        } el1;

        struct {                                // EL2 Registers
            uint64  hcr         { 0 };          // Hypervisor Configuration Register
            uint64  hpfar       { 0 };          // Hypervisor IPA Fault Address Register
            uint64  vdisr       { 0 };          // Virtual Deferred Interrupt Status Register
            uint64  vmpidr      { 0 };          // Virtualization Multiprocessor ID Register
            uint64  vpidr       { 0 };          // Virtualization Processor ID Register
        } el2;

        struct {                                // AArch32 Registers
            uint32  dacr        { 0 };          // Domain Access Control Register
            uint32  fpexc       { 0 };          // Floating-Point Exception Control Register
            uint32  hstr        { 0 };          // Hypervisor System Trap Register
            uint32  ifsr        { 0 };          // Instruction Fault Status Register
            uint32  spsr_abt    { 0 };          // Saved Program Status Register (Abort Mode)
            uint32  spsr_fiq    { 0 };          // Saved Program Status Register (FIQ Mode)
            uint32  spsr_irq    { 0 };          // Saved Program Status Register (IRQ Mode)
            uint32  spsr_und    { 0 };          // Saved Program Status Register (Undefined Mode)
        } a32;

        struct {                                // Timer Registers
            uint64  cntvoff     { 0 };          // Virtual Offset Register
            uint64  cntkctl     { 0 };          // Kernel Control Register
            uint64  cntv_cval   { 0 };          // Virtual Timer CompareValue Register
            uint64  cntv_ctl    { 0 };          // Virtual Timer Control Register
            bool    cntv_act    { false };      // Virtual Timer PPI Active State
        } tmr;

        struct {                                // GIC Registers
            uint64  lr[16]      { 0 };          // List Registers
            uint32  ap0r[4]     { 0 };          // Active Priorities Group 0 Registers
            uint32  ap1r[4]     { 0 };          // Active Priorities Group 1 Registers
            uint32  elrsr       { 0 };          // Empty List Register Status Register
            uint32  vmcr        { 0 };          // Virtual Machine Control Register
            uint32  hcr         { BIT (0) };    // Hypervisor Control Register
        } gic;

        static Vmcb const *current CPULOCAL;

        ALWAYS_INLINE
        inline void save_tmr()
        {
            tmr.cntv_act = Interrupt::get_act_tmr();
        }

        ALWAYS_INLINE
        inline void load_tmr() const
        {
            Interrupt::set_act_tmr (tmr.cntv_act);
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
        [[nodiscard]] ALWAYS_INLINE
        static inline void *operator new (size_t) noexcept
        {
            static_assert (sizeof (Vmcb) <= PAGE_SIZE);
            return Buddy::alloc (0);
        }

        /*
         * Deallocate VMCB
         *
         * @param ptr   Pointer to the VMCB (or nullptr)
         */
        ALWAYS_INLINE
        static inline void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                Buddy::free (ptr);
        }
};
