/*
 * Execution Context (EC)
 *
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

#include "assert.hpp"
#include "ec.hpp"
#include "mtd.hpp"
#include "utcb.hpp"

class Ec_arch final : private Ec
{
    friend class Ec;

    private:
        static void handle_irq_kern() asm ("irq_kern_handler");

        NORETURN
        static void handle_irq_user() asm ("irq_user_handler");

        NORETURN
        static void handle_exc_kern (Exc_regs *) asm ("exc_kern_handler");

        NORETURN
        static void handle_exc_user (Exc_regs *) asm ("exc_user_handler");

        NORETURN
        static void ret_user_hypercall();

        NORETURN
        static void ret_user_exception();

        NORETURN
        static void ret_user_vmexit();

        NORETURN
        static void set_vmm_info();

        ALWAYS_INLINE
        inline void state_load (Mtd_arch mtd)
        {
            assert (cont == ret_user_vmexit || cont == ret_user_exception);

            current->utcb->arch().load (mtd, cpu_regs());
        }

        ALWAYS_INLINE
        inline bool state_save (Mtd_arch mtd)
        {
            assert (cont == ret_user_vmexit || cont == ret_user_exception);

            return current->utcb->arch().save (mtd, cpu_regs());
        }

        NORETURN ALWAYS_INLINE
        inline void make_current()
        {
            // Become current EC and publish the pointer without store tearing
            __atomic_store_n (&current, this, __ATOMIC_RELAXED);

            // Invoke continuation function
            asm volatile ("mov sp, %0; br %1" : : "r" (CPU_LOCAL_STCK + PAGE_SIZE), "r" (cont) : "memory"); UNREACHED;
        }
};
