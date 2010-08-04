/*
 * Execution Context
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
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

#pragma once

#include "compiler.h"
#include "counter.h"
#include "fpu.h"
#include "lock_guard.h"
#include "mtd.h"
#include "pd.h"
#include "queue.h"
#include "refptr.h"
#include "regs.h"
#include "sc.h"
#include "tss.h"

class Utcb;

class Ec : public Kobject, public Queue<Sc>
{
    friend class Queue<Ec>;

    private:
        Exc_regs    regs;                       // 0x4, must be first
        void        (*cont)();                  // 0x50
        Ec *        reply;                      // 0x54
        Mtd         mtr;
        Utcb *      utcb;
        Refptr<Pd>  pd;
        Sc *        sc;
        Ec *        partner;
        Ec *        prev;
        Ec *        next;
        Fpu *       fpu;
        mword const cpu;
        mword const evt;
        mword       hazard;

        // EC Cache
        static Slab_cache cache;

        HOT NORETURN REGPARM (1)
        static void handle_sys (uint8) asm ("sys_handler");

        REGPARM (1)
        static void handle_exc (Exc_regs *) asm ("exc_handler");

        NORETURN
        static void handle_vmx() asm ("vmx_handler");

        NORETURN
        static void handle_svm() asm ("svm_handler");

        NORETURN
        static void handle_tss() asm ("tss_handler");

        static void handle_exc_nm();
        static bool handle_exc_ts (Exc_regs *);
        static bool handle_exc_gp (Exc_regs *);
        static bool handle_exc_pf (Exc_regs *);

        NORETURN
        static inline void vmx_exception();

        NORETURN
        static inline void vmx_extint();

        NORETURN
        static inline void vmx_invlpg();

        NORETURN
        static inline void vmx_cr();

        static bool fixup (mword &);

        NOINLINE
        static void handle_hazard (void (*)());

        ALWAYS_INLINE
        inline void set_partner (Ec *p)
        {
            partner = p;
            partner->reply = this;
            Sc::ctr_link++;
        }

        ALWAYS_INLINE
        inline unsigned clr_partner()
        {
            assert (partner == current);
            partner->reply = 0;
            partner = 0;
            return Sc::ctr_link--;
        }

    public:
        static Ec *current CPULOCAL_HOT;
        static Ec *fpowner CPULOCAL;

        Ec (Pd *, mword, Pd *, mword, mword, void (*)());            // Kernel EC
        Ec (Pd *, mword, Pd *, mword, mword, mword, mword, bool);    // Regular EC

        ALWAYS_INLINE
        inline bool set_sc (Sc *s)
        {
            return Atomic::cmp_swap<true>(&sc, static_cast<Sc *>(0), s);
        }

        ALWAYS_INLINE
        inline bool blocked() const { return next; }

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
                          : : "g" (KSTCK_ADDR + PAGE_SIZE), "rm" (cont) : "memory"); UNREACHED;
        }

        ALWAYS_INLINE
        static inline Ec *remote (unsigned c)
        {
            return *reinterpret_cast<volatile typeof current *>(reinterpret_cast<mword>(&current) - CPULC_ADDR + CPUGL_ADDR + c * PAGE_SIZE);
        }

        ALWAYS_INLINE
        inline void set_cont (void (*c)())
        {
            cont = c;
        }

        NOINLINE NORETURN
        void help (void (*c)())
        {
            Counter::print (++Counter::helping, Console_vga::COLOR_LIGHT_WHITE, SPN_HLP);
            current->cont = c;

            if (++Sc::ctr_loop >= 100) {
                trace (0, "Helping livelock detected on SC:%p", Sc::current);
                Sc::schedule (true);
            }

            activate (this);
        }

        NOINLINE
        void block()
        {
            {   Lock_guard <Spinlock> guard (lock);

                if (!blocked())
                    return;

                // Block SC on EC
                enqueue (Sc::current);
            }

            Sc::schedule (true);
        }

        ALWAYS_INLINE
        inline void release()
        {
            Lock_guard <Spinlock> guard (lock);

            for (Sc *s; (s = dequeue()); s->remote_enqueue()) ;
        }

        HOT NORETURN
        static void ret_user_sysexit();

        NORETURN
        static void ret_user_iret() asm ("ret_user_iret");

        NORETURN
        static void ret_user_vmresume();

        NORETURN
        static void ret_user_vmrun();

        template <Sys_regs::Status T>
        NOINLINE NORETURN
        static void sys_finish();

        NORETURN
        static void activate (Ec *);

        template <void (*)()>
        NORETURN
        static void send_msg();

        HOT NORETURN
        static void recv_msg();

        HOT NORETURN
        static void wait_msg();

        HOT NORETURN
        static void sys_call();

        HOT NORETURN
        static void sys_reply();

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
        static void sys_lookup();

        NORETURN
        static void sys_recall();

        NORETURN
        static void sys_semctl();

        NORETURN
        static void sys_assign_pci();

        NORETURN
        static void sys_assign_gsi();

        NORETURN
        static void idle();

        NORETURN
        static void root_invoke();

        template <bool, void (*)()>
        NORETURN
        static void delegate();

        NORETURN
        static void die() { die ("Dead EC"); }

        NORETURN
        static void die (char const *, Exc_regs * = &current->regs);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};
