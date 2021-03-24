/*
 * Execution Context (EC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "atomic.hpp"
#include "fpu.hpp"
#include "kmem.hpp"
#include "kobject.hpp"
#include "lock_guard.hpp"
#include "queue.hpp"
#include "regs.hpp"
#include "sc.hpp"
#include "slab.hpp"
#include "status.hpp"
#include "timeout_hypercall.hpp"

class Pd;
class Utcb;

class Ec : private Kobject, private Queue<Sc>, public Queue<Ec>::Element
{
    friend class Ec_arch;

    private:
        typedef void (*cont_t)(Ec *);   // Continuation Type

        Cpu_regs            regs;
        unsigned      const cpu         { 0 };
        unsigned long const evt         { 0 };
        Pd *          const pd          { nullptr };
        Fpu *         const fpu         { nullptr };
        Utcb *        const utcb        { nullptr };
        Ec *                callee      { nullptr };
        Ec *                caller      { nullptr };
        Atomic<cont_t>      cont        { nullptr };
        Atomic<unsigned>    hazard      { 0 };
        Timeout_hypercall   timeout     { this };
        Spinlock            lock;

        static Atomic<Ec *> current asm ("current") CPULOCAL;
        static Ec *         fpowner                 CPULOCAL;
        static unsigned     donations               CPULOCAL;
        static Slab_cache   cache;

        ALWAYS_INLINE inline auto &cpu_regs() { return regs; }
        ALWAYS_INLINE inline auto &exc_regs() { return regs.exc; }
        ALWAYS_INLINE inline auto &sys_regs() { return regs.exc.sys; }

        ALWAYS_INLINE
        inline bool is_vcpu() const { return subtype >= Kobject::Subtype::EC_VCPU_REAL; }

        ALWAYS_INLINE
        static inline Ec *remote_current (unsigned cpu)
        {
            return *Kmem::loc_to_glob (&current, cpu);
        }

        ALWAYS_INLINE NONNULL
        inline void set_partner (Ec *e)
        {
            callee = e;
            callee->caller = this;
            donations++;
        }

        ALWAYS_INLINE
        inline bool clr_partner()
        {
            callee->caller = nullptr;
            callee = nullptr;
            return donations--;
        }

        ALWAYS_INLINE
        inline bool tas_hazard (unsigned h) { return hazard.fetch_or (h) & h; }

        ALWAYS_INLINE
        inline void set_hazard (unsigned h) { hazard |= h; }

        ALWAYS_INLINE
        inline void clr_hazard (unsigned h) { hazard &= ~h; }

        void fpu_load();
        void fpu_save();

        static bool vcpu_supported();

        NOINLINE
        void handle_hazard (unsigned, cont_t);

        NOINLINE
        void help (Ec *, cont_t);

        ALWAYS_INLINE
        inline void rendezvous (Ec *, cont_t, cont_t, uintptr_t, uintptr_t, uintptr_t);

        [[noreturn]] HOT
        void reply (cont_t = nullptr);

        [[noreturn]]
        void kill (char const *);

        [[noreturn]]
        static void dead (Ec *self) { self->kill ("IPC Abort"); }

        [[noreturn]]
        static void blocking (Ec *self) { self->kill ("Blocking"); }

        [[noreturn]]
        static void idle (Ec *);

        [[noreturn]]
        static void root_invoke (Ec *);

        [[noreturn]] HOT
        static void recv_kern (Ec *);

        [[noreturn]] HOT
        static void recv_user (Ec *);

        template <cont_t>
        [[noreturn]]
        static void send_msg (Ec *);

        [[noreturn]] HOT
        static void sys_ipc_call (Ec *);

        [[noreturn]] HOT
        static void sys_ipc_reply (Ec *);

        [[noreturn]]
        static void sys_create_pd (Ec *);

        [[noreturn]]
        static void sys_create_ec (Ec *);

        [[noreturn]]
        static void sys_create_sc (Ec *);

        [[noreturn]]
        static void sys_create_pt (Ec *);

        [[noreturn]]
        static void sys_create_sm (Ec *);

        [[noreturn]]
        static void sys_ctrl_pd (Ec *);

        [[noreturn]]
        static void sys_ctrl_ec (Ec *);

        [[noreturn]]
        static void sys_ctrl_sc (Ec *);

        [[noreturn]]
        static void sys_ctrl_pt (Ec *);

        [[noreturn]]
        static void sys_ctrl_sm (Ec *);

        [[noreturn]]
        static void sys_ctrl_hw (Ec *);

        [[noreturn]]
        static void sys_assign_int (Ec *);

        [[noreturn]]
        static void sys_assign_dev (Ec *);

        // Constructor: Kernel Thread
        Ec (Pd *p, unsigned c, cont_t x) : Kobject (Kobject::Type::EC, Kobject::Subtype::EC_GLOBAL), cpu (c), pd (p), cont (x) {}

        // Constructor: User Thread
        Ec (Pd *p, Fpu *f, Utcb *u, unsigned c, unsigned long e, cont_t x) : Kobject (Kobject::Type::EC, x ? Kobject::Subtype::EC_GLOBAL : Kobject::Subtype::EC_LOCAL), cpu (c), evt (e), pd (p), fpu (f), utcb (u), cont (x) {}

        // Constructor: Virtual CPU
        template <typename T>
        Ec (Pd *p, Fpu *f, T *v, unsigned c, unsigned long e, cont_t x, bool t) : Kobject (Kobject::Type::EC, t ? Kobject::Subtype::EC_VCPU_OFFS : Kobject::Subtype::EC_VCPU_REAL), regs (v), cpu (c), evt (e), pd (p), fpu (f), cont (x) {}

        [[nodiscard]]
        static inline void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }

        static inline void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                cache.free (ptr);
        }

    public:
        // Factory: Kernel Thread
        [[nodiscard]]
        static Ec *create (unsigned, cont_t);

        // Factory: User Thread
        [[nodiscard]]
        static Ec *create (Pd *, bool, unsigned, unsigned long, uintptr_t, uintptr_t, cont_t);

        // Factory: Virtual CPU
        [[nodiscard]]
        static Ec *create (Pd *, bool, unsigned, unsigned long, bool);

        static void create_idle();
        static void create_root();

        static bool switch_fpu (Ec *);

        void destroy() { delete this; }

        ALWAYS_INLINE
        inline bool blocked() const { cont_t c = cont; return c == blocking || c == nullptr; }

        ALWAYS_INLINE
        inline void block() { cont = blocking; }

        ALWAYS_INLINE
        inline void unblock (cont_t c) { cont = c; }

        /*
         * Core X               Core Y
         * e.g. Sm::dn()        e.g. Sm::up()
         *
         * A: ec->block()       C: ec->unblock()
         * B: ec->block_sc()    D: ec->unblock_sc()
         *
         * Ordering: A before B, C before D, A before C, B+D can't run in parallel
         *
         * @return true if B happened before C, false if B happened after C
         */
        [[nodiscard]]
        bool block_sc()
        {
            {   Lock_guard <Spinlock> guard (lock);

                // If C already happened, then don't block the SC
                if (!blocked())
                    return false;

                // Otherwise D will later unblock the SC
                enqueue_tail (Scheduler::get_current());
            }

            return true;
        }

        ALWAYS_INLINE
        void unblock_sc()
        {
            Lock_guard <Spinlock> guard (lock);

            for (Sc *sc; (sc = dequeue_head()); Scheduler::unblock (sc)) ;
        }

        ALWAYS_INLINE
        inline void set_timeout (uint64 t, Sm *s)
        {
            timeout.enqueue (t, s);
        }

        ALWAYS_INLINE
        inline void clr_timeout()
        {
            timeout.dequeue();
        }

        void activate();

        void adjust_offset_ticks (uint64);

        template <Status S, bool T = false>
        [[noreturn]] NOINLINE
        static void sys_finish (Ec *);

        static cont_t const syscall[16] asm ("syscall");
};
