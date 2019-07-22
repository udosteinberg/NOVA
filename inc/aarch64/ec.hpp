/*
 * Execution Context (EC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

class Ec : private Kobject, public Queue<Sc>
{
    friend class Ec_arch;
    friend class Queue<Ec>;

    private:
        Cpu_regs            regs;
        unsigned      const cpu         { 0 };
        unsigned long const evt         { 0 };
        Pd *          const pd          { nullptr };
        Fpu *         const fpu         { nullptr };
        Utcb *        const utcb        { nullptr };
        void                (*cont)()   { nullptr };
        Ec *                prev        { nullptr };
        Ec *                next        { nullptr };
        unsigned            hazard      { 0 };
        Timeout_hypercall   timeout     { this };
        Spinlock            lock;

        static Slab_cache   cache;

        ALWAYS_INLINE
        inline Cpu_regs *cpu_regs() { return &regs; }

        ALWAYS_INLINE
        inline Exc_regs *exc_regs() { return &regs; }

        ALWAYS_INLINE
        inline Sys_regs *sys_regs() { return &regs; }

        void fpu_load();
        void fpu_save();
        bool fpu_switch();

        static bool vcpu_supported();

        NOINLINE
        static void handle_hazard (unsigned, void (*)());

        Ec (unsigned, void (*)());

        Ec (Pd *, Fpu *, Utcb *, unsigned, mword, mword, mword, void (*)());

        template <typename T>
        Ec (Pd *, Fpu *, T *, unsigned, mword);

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
        static Ec *current  CPULOCAL;
        static Ec *fpowner  CPULOCAL;

        NODISCARD
        static Ec *create (unsigned c, void (*x)())
        {
            return new Ec (c, x);
        }

        NODISCARD
        static Ec *create (Pd *, bool, unsigned, mword, mword, mword, void (*)());

        NODISCARD
        static Ec *create (Pd *, bool, unsigned, mword);

        void destroy() { delete this; }

        ALWAYS_INLINE
        static inline auto remote_current (unsigned cpu)
        {
            return __atomic_load_n (reinterpret_cast<decltype (current) *>(reinterpret_cast<uintptr_t>(&current) - CPU_LOCAL_DATA + CPU_GLOBL_DATA + cpu * PAGE_SIZE), __ATOMIC_RELAXED);
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
            timeout.enqueue (t, s);
        }

        ALWAYS_INLINE
        inline void clr_timeout()
        {
            timeout.dequeue();
        }

        void activate();

        void adjust_offset_ticks (uint64);

        ALWAYS_INLINE
        inline bool blocked() const { return next || !cont; }

        NORETURN
        static void idle();

        NORETURN
        static void kill (char const *);
};
