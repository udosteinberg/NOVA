/*
 * Execution Context
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
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
#include "regs.h"
#include "sc.h"
#include "tss.h"

class Utcb;

class Ec : public Kobject, public Refcount, public Queue<Sc>
{
    friend class Queue<Ec>;

    private:
        void        (*cont)();
        Cpu_regs    regs;
        Ec *        rcap;
        Utcb *      utcb;
        Refptr<Pd>  pd;
        Ec *        partner;
        Ec *        prev;
        Ec *        next;
        Fpu *       fpu;
        union {
            struct {
                uint16  cpu;
                uint16  glb;
            };
            uint32  xcpu;
        };
        unsigned const evt;

        static Slab_cache cache;

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

        static inline uint8 ifetch (mword);

        NORETURN
        static inline void svm_exception (mword);

        NORETURN
        static inline void svm_cr();

        NORETURN
        static inline void svm_invlpg();

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
        static void handle_hazard (mword, void (*)());

        ALWAYS_INLINE
        inline Sys_regs *sys_regs() { return &regs; }

        ALWAYS_INLINE
        inline Exc_regs *exc_regs() { return &regs; }

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
            partner->rcap = 0;
            partner = 0;
            return Sc::ctr_link--;
        }

    public:
        static Ec *current CPULOCAL_HOT;
        static Ec *fpowner CPULOCAL;

        Ec (Pd *, mword, void (*)(), unsigned);
        Ec (Pd *, mword, void (*)(), unsigned, unsigned, mword, mword);

        ALWAYS_INLINE
        inline void add_tsc_offset (uint64 tsc)
        {
            regs.add_tsc_offset (tsc);
        }

        ALWAYS_INLINE
        inline bool blocked() const { return next || !cont; }

        ALWAYS_INLINE NORETURN
        inline void make_current()
        {
            current = this;

            Tss::run.sp0 = reinterpret_cast<mword>(exc_regs() + 1);

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

        NOINLINE
        void help (void (*c)())
        {
            if (EXPECT_TRUE (cont != dead)) {

                Counter::print (++Counter::helping, Console_vga::COLOR_LIGHT_WHITE, SPN_HLP);
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
        void activate();

        template <void (*)()>
        NORETURN
        static void send_msg();

        HOT NORETURN
        static void recv_kern();

        HOT NORETURN
        static void recv_user();

        HOT NORETURN
        static void reply (void (*)() = 0);

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

        template <bool>
        NORETURN
        static void delegate();

        NORETURN
        static void dead() { die ("IPC Abort"); }

        NORETURN
        static void die (char const *, Exc_regs * = &current->regs);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};
