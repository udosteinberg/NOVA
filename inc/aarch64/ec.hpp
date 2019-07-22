/*
 * Execution Context (EC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
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

#include "compiler.hpp"
#include "fpu.hpp"
#include "kobject.hpp"
#include "lock_guard.hpp"
#include "pd.hpp"
#include "queue.hpp"
#include "regs.hpp"
#include "sc.hpp"
#include "slab.hpp"
#include "timeout_hypercall.hpp"
#include "utcb.hpp"
#include "vmcb.hpp"

class Ec : private Kobject, public Queue<Sc>
{
    friend class Queue<Ec>;

    private:
        Exc_regs            regs            ALIGNED (16);
        unsigned const      cpu             { 0 };
        unsigned long const evt             { 0 };
        Pd *     const      pd              { nullptr };
        Fpu *    const      fpu             { nullptr };
        union {
            Utcb * const    utcb            { nullptr };
            Vmcb * const    vmcb;
        };
        void                (*cont)()       { nullptr };
        Ec *                prev            { nullptr };
        Ec *                next            { nullptr };
        unsigned            hazard          { 0 };
        Timeout_hypercall   timeout         { this };

        static Slab_cache   cache;

        ALWAYS_INLINE
        inline Exc_regs *exc_regs() { return &regs; }

        ALWAYS_INLINE
        inline Sys_regs *sys_regs() { return &regs; }

        NORETURN
        static void set_vmm_info();

        NOINLINE
        static void handle_hazard (unsigned, void (*)());

        bool switch_fpu();

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

        Ec (unsigned, void (*)());
        Ec (Pd *, Fpu *, Utcb *, unsigned, mword, mword, mword, void (*)());
        Ec (Pd *, Fpu *, Vmcb *, unsigned, mword, void (*)());

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }

    public:
        static Ec *current  CPULOCAL;
        static Ec *fpowner  CPULOCAL;

        static Ec *create (unsigned c, void (*x)())
        {
            Ec *ptr (new Ec (c, x));
            return ptr;
        }

        static Ec *create (Pd *p, Fpu *f, Utcb *u, unsigned c, mword e, mword a, mword s, void (*x)())
        {
            Ec *ptr (new Ec (p, f, u, c, e, a, s, x));
            return ptr;
        }

        static Ec *create (Pd *p, Fpu *f, Vmcb *v, unsigned c, mword e, void (*x)())
        {
            Ec *ptr (new Ec (p, f, v, c, e, x));
            return ptr;
        }

        void destroy() { delete this; }

        ALWAYS_INLINE
        inline bool blocked() const { return next || !cont; }

        void activate();

        ALWAYS_INLINE
        static auto remote_current (unsigned cpu)
        {
            return Atomic::load (*reinterpret_cast<decltype (current) *>(reinterpret_cast<mword>(&current) - CPU_LOCAL_DATA + CPU_GLOBL_DATA + cpu * PAGE_SIZE));
        }

        ALWAYS_INLINE NORETURN
        inline void make_current()
        {
            // Become current EC
            current = this;

            // Invoke continuation function
            asm volatile ("mov sp, %0; br %1" : : "r" (CPU_LOCAL_STCK + PAGE_SIZE), "r" (cont) : "memory"); UNREACHED;
        }

        WARN_UNUSED_RESULT
        bool block_sc()
        {
            {   Lock_guard <Spinlock> guard (lock);

                if (!blocked())
                    return false;

                enqueue (Sc::current);
            }

            return true;
        }

        ALWAYS_INLINE
        inline void release (void (*c)())
        {
            cont = c;

            Lock_guard <Spinlock> guard (lock);

            for (Sc *s; dequeue (s = head()); s->remote_enqueue()) ;
        }

        ALWAYS_INLINE
        inline void set_hazard (unsigned h) { Atomic::set_mask (hazard, h); }

        ALWAYS_INLINE
        inline void clr_hazard (unsigned h) { Atomic::clr_mask (hazard, h); }

        ALWAYS_INLINE
        inline void set_timeout (uint64 t, Sm *s)
        {
            if (EXPECT_FALSE (t))
                timeout.enqueue (t, s);
        }

        ALWAYS_INLINE
        inline void clr_timeout()
        {
            if (EXPECT_FALSE (timeout.active()))
                timeout.dequeue();
        }

        ALWAYS_INLINE
        inline void add_offset_ticks (uint64 t)
        {
            if (subtype == Kobject::Subtype::EC_VCPU)
                vmcb->tmr.cntvoff += t;
        }

        NORETURN
        static void idle();

        NORETURN
        static void kill (char const *);
};
