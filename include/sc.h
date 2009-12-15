/*
 * Scheduling Context
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

#include "compiler.h"
#include "kobject.h"
#include "slab.h"
#include "types.h"

class Ec;

class Sc : public Kobject
{
    template <typename T> friend class Queue;

    private:
        Ec * const      owner;
        Sc *            prev;
        Sc *            next;
        unsigned long   cpu;
        unsigned long   prio;
        unsigned long   full;
        unsigned long   left;

        static unsigned const priorities = 256;

        // SC Cache
        static Slab_cache cache;

        // Ready List
        static Sc *list[priorities] CPULOCAL;

        static unsigned long prio_top CPULOCAL;

        // Release Queue
        static struct Rq {
            Spinlock    lock;
            Sc *        queue;
        } release[NUM_CPU];

    public:
        // Current SC
        static Sc *     current     CPULOCAL_HOT;
        static unsigned ctr_link    CPULOCAL;
        static unsigned ctr_loop    CPULOCAL;

        static unsigned long const default_prio = 1;
        static unsigned long const default_quantum = 10000;

        Sc (Ec *, unsigned long, unsigned long, unsigned long);

        void ready_enqueue();
        void ready_dequeue();

        void remote_enqueue();
        static void remote_enqueue_handler();

        NORETURN
        static void schedule (bool = false);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};
