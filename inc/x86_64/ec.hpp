/*
 * Execution Context
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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

#include "atomic.hpp"
#include "counter.hpp"
#include "extern.hpp"
#include "fpu.hpp"
#include "kmem.hpp"
#include "lock_guard.hpp"
#include "mtd.hpp"
#include "pd.hpp"
#include "queue.hpp"
#include "regs.hpp"
#include "sc.hpp"
#include "timeout_hypercall.hpp"
#include "tss.hpp"

class Utcb;

class Ec : private Kobject, private Queue<Sc>, public Queue<Ec>::Element
{
    friend class Tlb;

    private:
        void        (*cont)() ALIGNED (16);
        Cpu_regs    regs;
        Ec *        rcap;
        Utcb *      utcb;
        Pd * const  pd;
        Ec *        partner;
        Fpu *       fpu;
        union {
            struct {
                uint16  cpu;
                uint16  glb;
            };
            uint32  xcpu;
        };
        unsigned const evt;
        Timeout_hypercall timeout;
        Spinlock    lock;

        static Slab_cache cache;

        static void handle_exc (Exc_regs *) asm ("exc_handler");

        [[noreturn]]
        static void handle_vmx() asm ("vmx_handler");

        [[noreturn]]
        static void failed_vmx() asm ("vmx_failure");

        [[noreturn]]
        static void handle_svm() asm ("svm_handler");

        static void handle_exc_nm();
        static bool handle_exc_gp (Exc_regs *);
        static bool handle_exc_pf (Exc_regs *);

        [[noreturn]] static void svm_exception (uint64_t);

        [[noreturn]]
        static inline void vmx_exception();

        [[noreturn]]
        static inline void vmx_extint();

        NOINLINE
        static void handle_hazard (mword, void (*)());

        ALWAYS_INLINE inline Cpu_regs &cpu_regs() { return regs; }
        ALWAYS_INLINE inline Exc_regs &exc_regs() { return regs.exc; }
        ALWAYS_INLINE inline Sys_regs &sys_regs() { return regs.exc.sys; }

        ALWAYS_INLINE
        inline void set_partner (Ec *p)
        {
            partner = p;
            partner->rcap = this;
            Sc::ctr_link++;
        }

        ALWAYS_INLINE
        inline unsigned clr_partner()
        {
            assert (partner == current);
            partner->rcap = nullptr;
            partner = nullptr;
            return Sc::ctr_link--;
        }

        ALWAYS_INLINE
        inline void redirect_to_iret()
        {
            exc_regs().rsp = exc_regs().sp();
            exc_regs().rip = exc_regs().ip();
        }

        void load_fpu();
        void save_fpu();

        void transfer_fpu (Ec *);

        [[noreturn]]
        void sys_finish_status (Status);

    public:
        static Ec *current CPULOCAL_HOT;
        static Ec *fpowner CPULOCAL;

        Ec (Pd *, void (*)(), cpu_t);
        Ec (Pd *, mword, Pd *, void (*)(), cpu_t, unsigned, mword, mword);

        ALWAYS_INLINE
        inline void add_tsc_offset (uint64 tsc)
        {
            regs.exc.offset_tsc += tsc;
            regs.hazard.set (Hazard::TSC);
        }

        ALWAYS_INLINE
        inline bool blocked() const { return queued() || !cont; }

        ALWAYS_INLINE
        inline void set_timeout (uint64 t, Sm *s)
        {
            if (EXPECT_FALSE (t))
                timeout.enqueue (t, s);
        }

        ALWAYS_INLINE
        inline void clr_timeout()
        {
            timeout.dequeue();
        }

        ALWAYS_INLINE
        static inline Ec *remote_current (unsigned c)
        {
            return *Kmem::loc_to_glob (c, &current);
        }

        [[noreturn]] ALWAYS_INLINE
        inline void make_current()
        {
            current = this;

            Tss::run.rsp[0] = reinterpret_cast<uintptr_t>(&exc_regs() + 1);

            regs.get_hst()->make_current();

            asm volatile ("lea %0, %%rsp; jmp *%1" : : "m" (DSTK_TOP), "q" (cont) : "memory"); UNREACHED;
        }

        NOINLINE
        void help (void (*c)())
        {
            if (EXPECT_TRUE (cont != dead)) {

                Counter::helping.inc();

                current->cont = c;

                if (EXPECT_TRUE (++Sc::ctr_loop < 100))
                    activate();

                die ("Livelock");
            }
        }

        NOINLINE
        void block_sc()
        {
            {   Lock_guard <Spinlock> guard (lock);

                if (!blocked())
                    return;

                enqueue_tail (Sc::current);
            }

            Sc::schedule (true);
        }

        ALWAYS_INLINE
        inline void release (void (*c)())
        {
            cont = c;

            Lock_guard <Spinlock> guard (lock);

            for (Sc *sc; (sc = dequeue_head()); sc->remote_enqueue()) ;
        }

        [[noreturn]] HOT
        static void ret_user_sysexit();

        [[noreturn]] HOT
        static void ret_user_iret() asm ("ret_user_iret");

        [[noreturn]]
        static void ret_user_vmresume();

        [[noreturn]]
        static void ret_user_vmrun();

        template <Status S, bool T = false>
        [[noreturn]] NOINLINE
        static void sys_finish();

        [[noreturn]]
        void activate();

        template <void (*)()>
        [[noreturn]]
        static void send_msg();

        [[noreturn]] HOT
        static void recv_kern();

        [[noreturn]] HOT
        static void recv_user();

        [[noreturn]] HOT
        static void reply (void (*)() = nullptr);

        [[noreturn]] HOT
        static void sys_call();

        [[noreturn]] HOT
        static void sys_reply();

        [[noreturn]]
        static void sys_create_pd();

        [[noreturn]]
        static void sys_create_ec();

        [[noreturn]]
        static void sys_create_sc();

        [[noreturn]]
        static void sys_create_pt();

        [[noreturn]]
        static void sys_create_sm();

        [[noreturn]]
        static void sys_revoke();

        [[noreturn]]
        static void sys_lookup();

        [[noreturn]]
        static void sys_ctrl_pd();

        [[noreturn]]
        static void sys_ctrl_ec();

        [[noreturn]]
        static void sys_ctrl_sc();

        [[noreturn]]
        static void sys_ctrl_pt();

        [[noreturn]]
        static void sys_ctrl_sm();

        [[noreturn]]
        static void sys_ctrl_hw();

        [[noreturn]]
        static void sys_assign_int();

        [[noreturn]]
        static void sys_assign_dev();

        [[noreturn]]
        static void idle();

        [[noreturn]]
        static void root_invoke();

        [[noreturn]]
        static void dead() { die ("IPC Abort"); }

        [[noreturn]]
        static void die (char const *, Exc_regs * = &current->exc_regs());

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};
