/*
 * Execution Context (EC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "ec.hpp"
#include "pd.hpp"
#include "tss.hpp"

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

        static void handle_exc (Exc_regs *) asm ("exc_handler");

        NORETURN
        static void handle_vmx() asm ("vmx_handler");

        NORETURN
        static void handle_svm() asm ("svm_handler");

        bool handle_exc_gp (Exc_regs *);
        bool handle_exc_pf (Exc_regs *);

        NORETURN
        inline void svm_exception (mword);

        NORETURN
        inline void vmx_exception();

        NORETURN
        inline void vmx_extint();

        ALWAYS_INLINE
        inline void redirect_to_iret()
        {
            exc_regs().rsp = exc_regs().sp();
            exc_regs().rip = exc_regs().ip();
        }

        NORETURN
        static void ret_user_hypercall (Ec *);

        NORETURN
        static void ret_user_exception (Ec *) asm ("ret_user_iret");

        NORETURN
        static void ret_user_vmexit_vmx (Ec *);

        NORETURN
        static void ret_user_vmexit_svm (Ec *);

        ALWAYS_INLINE
        inline void state_load (Ec *const self, Mtd_arch mtd)
        {
            assert (cont == ret_user_vmexit_vmx || cont == ret_user_vmexit_svm || cont == ret_user_exception);

            auto state = self->utcb->arch();

            if (cont == ret_user_vmexit_vmx)
                state->load_vmx (mtd, cpu_regs());
            else if (cont == ret_user_vmexit_svm)
                state->load_svm (mtd, cpu_regs());
            else // cont == ret_user_exception
                state->load_exc (mtd, exc_regs());
        }

        ALWAYS_INLINE
        inline bool state_save (Ec *const self, Mtd_arch mtd)
        {
            assert (cont == ret_user_vmexit_vmx || cont == ret_user_vmexit_svm || cont == ret_user_exception);

            auto state = self->utcb->arch();

            if (cont == ret_user_vmexit_vmx)
                return state->save_vmx (mtd, cpu_regs());
            else if (cont == ret_user_vmexit_svm)
                return state->save_svm (mtd, cpu_regs());
            else // cont == ret_user_exception
                return state->save_exc (mtd, exc_regs());
        }

        NORETURN ALWAYS_INLINE
        inline void make_current()
        {
            Tss::run.sp0 = reinterpret_cast<uintptr_t>(&exc_regs() + 1);

            assert (!(Tss::run.sp0 & 0xf));

            pd->make_current();

            // Reset stack
            asm volatile ("mov %0, %%rsp" : : "i" (MMAP_CPU_STCK + PAGE_SIZE) : "memory");

            // Become current EC and invoke continuation
            (*cont)(current = this);

            UNREACHED;
        }
};
