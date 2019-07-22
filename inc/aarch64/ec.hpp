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

#include "atomic.hpp"
#include "compiler.hpp"
#include "kobject.hpp"
#include "lock_guard.hpp"
#include "memory.hpp"
#include "queue.hpp"
#include "regs.hpp"
#include "sc.hpp"
#include "slab.hpp"
#include "timeout_hypercall.hpp"

class Fpu;
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
        Atomic<cont_t>      cont        { nullptr };
        Atomic<unsigned>    hazard      { 0 };
        Timeout_hypercall   timeout     { this };
        Spinlock            lock;

        static Atomic<Ec *> current     CPULOCAL;
        static Ec *         fpowner     CPULOCAL;
        static Slab_cache   cache;

        ALWAYS_INLINE
        inline Cpu_regs *cpu_regs() { return &regs; }

        ALWAYS_INLINE
        inline Exc_regs *exc_regs() { return &regs; }

        ALWAYS_INLINE
        inline Sys_regs *sys_regs() { return &regs; }

        ALWAYS_INLINE
        inline bool blocked() const { return queued() || !cont; }

        ALWAYS_INLINE
        inline void set_hazard (unsigned h) { hazard |= h; }

        ALWAYS_INLINE
        inline void clr_hazard (unsigned h) { hazard &= ~h; }

        void fpu_load();
        void fpu_save();
        bool fpu_switch();

        static bool vcpu_supported();

        NOINLINE
        void handle_hazard (unsigned, cont_t);

        NORETURN
        void kill (char const *);

        NORETURN
        static void idle (Ec *);

        // Kernel Thread
        Ec (unsigned, cont_t);

        // User Thread
        Ec (Pd *, Fpu *, Utcb *, unsigned, unsigned long, uintptr_t, uintptr_t, cont_t);

        // Virtual CPU
        template <typename T>
        Ec (Pd *, Fpu *, T *, unsigned, unsigned long);

        NODISCARD
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
        // Kernel Thread
        NODISCARD
        static Ec *create (unsigned c, cont_t x) { return new Ec (c, x); }

        // User Thread
        NODISCARD
        static Ec *create (Pd *, bool, unsigned, unsigned long, uintptr_t, uintptr_t, cont_t);

        // Virtual CPU
        NODISCARD
        static Ec *create (Pd *, bool, unsigned, unsigned long);

        static void create_idle();
        static void create_root();

        void destroy() { delete this; }

        ALWAYS_INLINE
        static inline Ec *remote_current (unsigned cpu)
        {
            return *reinterpret_cast<decltype (current) *>(reinterpret_cast<uintptr_t>(&current) - CPU_LOCAL_DATA + CPU_GLOBL_DATA + cpu * PAGE_SIZE);
        }

        NODISCARD
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
        inline void release (cont_t c)
        {
            cont = c;

            Lock_guard <Spinlock> guard (lock);

            for (Sc *sc; (sc = head()); sc->remote_enqueue())
                dequeue (sc);
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
};
