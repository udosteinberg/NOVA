/*
 * Scheduling Context
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

class Ec;

class Sc : public Kobject
{
    friend class Queue<Sc>;

    private:
        Ec * const      owner;
        Sc *            prev;
        Sc *            next;
        unsigned        cpu;
        unsigned long   prio;
        unsigned long   full;
        unsigned long   left;
        uint64          tsc;

        static unsigned const priorities = 256;

        static Slab_cache cache;

        static struct Rq {
            Spinlock    lock;
            Sc *        queue;
        } rq CPULOCAL;

        static Sc *list[priorities] CPULOCAL;

        static unsigned long prio_top CPULOCAL;

        void ready_enqueue();
        void ready_dequeue();

    public:
        static Sc *     current     CPULOCAL_HOT;
        static unsigned ctr_link    CPULOCAL;
        static unsigned ctr_loop    CPULOCAL;

        static unsigned long const default_prio = 1;
        static unsigned long const default_quantum = 10000;

        Sc (Pd *, mword, Ec *, unsigned, mword, mword);

        ALWAYS_INLINE
        inline Ec *ec() const { return owner; }

        ALWAYS_INLINE
        static inline Rq *remote (unsigned long c)
        {
            return reinterpret_cast<typeof rq *>(reinterpret_cast<mword>(&rq) - CPULC_ADDR + CPUGL_ADDR + c * PAGE_SIZE);
        }

        void remote_enqueue();

        static void rrq_handler();
        static void rke_handler();

        NORETURN
        static void schedule (bool = false);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};
