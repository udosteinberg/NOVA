/*
 * Scheduling Context
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

class Ec;

class Sc : public Kobject
{
    friend class Queue<Sc>;

    public:
        Refptr<Ec> const ec;
        unsigned const cpu;
        unsigned const prio;
        uint64 const budget;
        uint64 time;

    private:
        uint64 left;
        Sc *prev, *next;
        uint64 tsc;

        static unsigned const priorities = 128;

        static Slab_cache cache;

        static struct Rq {
            Spinlock    lock;
            Sc *        queue;
        } rq CPULOCAL;

        static Sc *list[priorities] CPULOCAL;

        static unsigned prio_top CPULOCAL;

        void ready_enqueue (uint64);
        void ready_dequeue (uint64);

    public:
        static Sc *     current     CPULOCAL_HOT;
        static unsigned ctr_link    CPULOCAL;
        static unsigned ctr_loop    CPULOCAL;

        static unsigned const default_prio = 1;
        static unsigned const default_quantum = 10000;

        Sc (Pd *, mword, Ec *);
        Sc (Pd *, mword, Ec *, unsigned, unsigned, unsigned);

        ALWAYS_INLINE
        static inline Rq *remote (unsigned long c)
        {
            return reinterpret_cast<typeof rq *>(reinterpret_cast<mword>(&rq) - CPU_LOCAL_DATA + HV_GLOBAL_CPUS + c * PAGE_SIZE);
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
