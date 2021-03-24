/*
 * Execution Context (EC)
 *
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

#include "assert.hpp"
#include "ec.hpp"
#include "extern.hpp"
#include "mtd.hpp"
#include "utcb.hpp"

class Ec_arch final : private Ec
{
    friend class Ec;

    private:
        // Constructor: Kernel Thread
        Ec_arch (Pd *, unsigned, cont_t);

        // Constructor: User Thread
        Ec_arch (Pd *, Fpu *, Utcb *, unsigned, unsigned long, uintptr_t, uintptr_t, cont_t);

        // Constructor: Virtual CPU
        template <typename T>
        Ec_arch (Pd *, Fpu *, T *, unsigned, unsigned long, bool);

        static void handle_irq_kern() asm ("irq_kern_handler");

        NORETURN
        static void handle_irq_user() asm ("irq_user_handler");

        NORETURN
        static void handle_exc_kern (Exc_regs *) asm ("exc_kern_handler");

        NORETURN
        static void handle_exc_user (Exc_regs *) asm ("exc_user_handler");

        NORETURN
        static void ret_user_hypercall (Ec *);

        NORETURN
        static void ret_user_exception (Ec *);

        NORETURN
        static void ret_user_vmexit (Ec *);

        NORETURN
        static void set_vmm_info (Ec *);

        ALWAYS_INLINE
        inline void state_load (Ec *const self, Mtd_arch mtd)
        {
            assert (cont == ret_user_vmexit || cont == ret_user_exception);

            self->utcb->arch()->load (mtd, cpu_regs());
        }

        ALWAYS_INLINE
        inline bool state_save (Ec *const self, Mtd_arch mtd)
        {
            assert (cont == ret_user_vmexit || cont == ret_user_exception);

            return self->utcb->arch()->save (mtd, cpu_regs());
        }

        NORETURN ALWAYS_INLINE
        inline void make_current()
        {
            uintptr_t dummy;

            // Reset stack
            asm volatile ("adrp %0, %1; mov sp, %0" : "=&r" (dummy) : "S" (&STCK_TOP) : "memory");

            // Become current EC and invoke continuation
            (*cont)(current = this);

            UNREACHED;
        }
};
