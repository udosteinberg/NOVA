/*
 * Execution Context
 *
 * Copyright (C) 2007-2009, Udo Steinberg <udo@hypervisor.org>
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

#include "capability.h"
#include "compiler.h"
#include "fpu.h"
#include "kobject.h"
#include "pd.h"
#include "queue.h"
#include "refptr.h"
#include "regs.h"
#include "slab.h"
#include "syscall.h"
#include "tss.h"
#include "types.h"

class Pt;
class Sc;
class Utcb;
class Vmcs;

class Ec : public Kobject
{
    private:
        Exc_regs regs;                              // 0x4, must be first
        void (*continuation)();                     // 0x50
        Capability reply;                           // 0x54
        Queue send;                                 // 0x58

        Utcb *      utcb;                           // 0x5c
        Refptr<Pd>  pd;                             // 0x60
        Sc *        sc;                             // 0x64
        Fpu *       fpu;
        mword       cpu;
        mword       evt;
        mword       wait;
        bool        worker;
        unsigned    hazard;

        // EC Cache
        static Slab_cache cache;

        REGPARM (1)
        static void exc_handler (Exc_regs *) asm ("exc_handler");

        NORETURN
        static void vmx_handler() asm ("vmx_handler");

        NORETURN
        static void svm_handler() asm ("svm_handler");

        NORETURN
        static inline void vmx_exception();

        NORETURN
        static inline void vmx_extint();

        NORETURN
        static inline void vmx_invlpg();

        NORETURN
        static inline void vmx_cr();

        NOINLINE
        static void handle_hazard (void (*)());

    public:
        static Ec *current CPULOCAL_HOT;
        static Ec *fpowner;

        Ec (Pd *, void (*)());                          // Kernel EC
        Ec (Pd *, mword, mword, mword, mword, bool);    // Regular EC

        ALWAYS_INLINE
        inline bool set_sc (Sc *s)
        {
            return !worker && Atomic::cmp_swap<true>(&sc, static_cast<Sc *>(0), s);
        }

        ALWAYS_INLINE NORETURN
        inline void make_current()
        {
            current = this;

            if (Fpu::enabled)
                Fpu::disable();

            Tss::run.sp0 = reinterpret_cast<mword>(&regs + 1);

            pd->make_current();

            asm volatile ("mov %0, %%esp;"
                          "jmp *%1"
                          : : "g" (KSTCK_ADDR + PAGE_SIZE), "g" (continuation) : "memory"); UNREACHED;
        }

        ALWAYS_INLINE
        inline void set_continuation (void (*c)())
        {
            continuation = c;
        }

        NOINLINE NORETURN
        void block (void (*c)())
        {
            send.block();
            current->continuation = c;
            Sc::schedule (true);
        }

        ALWAYS_INLINE
        inline bool release()
        {
            return send.release();
        }

        HOT NORETURN
        static void ret_user_sysexit();

        NORETURN
        static void ret_user_iret() asm ("ret_user_iret");

        NORETURN
        static void ret_user_vmresume();

        NORETURN
        static void ret_user_vmrun();

        ALWAYS_INLINE NORETURN
        static inline void sys_finish (Sys_regs *param, Sys_regs::Status status);

        NORETURN
        static void send_exc_msg();

        NORETURN
        static void send_vmx_msg();

        NORETURN
        static void send_svm_msg();

        ALWAYS_INLINE NORETURN
        inline void recv_ipc_msg (mword, unsigned = 0);

        HOT NORETURN
        static void sys_ipc_call();

        HOT NORETURN
        static void sys_ipc_reply();

        NORETURN
        static void sys_create_pd();

        NORETURN
        static void sys_create_ec();

        NORETURN
        static void sys_create_sc();

        NORETURN
        static void sys_create_pt();

        NORETURN
        static void sys_create_sm();

        NORETURN
        static void sys_revoke();

        NORETURN
        static void sys_recall();

        NORETURN
        static void sys_semctl();

        NORETURN
        static void exp_debug();

        NORETURN
        static void exp_lookup();

        NORETURN
        static void idle();

        NORETURN
        static void root_invoke();

        HOT NORETURN REGPARM (1)
        static void syscall_handler (uint8) asm ("syscall_handler");

        NORETURN
        static void preemption_handler() asm ("preemption_handler");

        NORETURN
        static void task_gate_handler() asm ("task_gate_handler");

        static void fpu_handler();
        static bool tss_handler (Exc_regs *);
        static bool pf_handler (Exc_regs *);

        NORETURN
        void kill (Exc_regs *, char const *);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};
