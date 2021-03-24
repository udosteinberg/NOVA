/*
 * Scheduling Context (SC)
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
#include "queue.hpp"
#include "slab.hpp"

class Ec;

class Sc : public Kobject, public Queue<Sc>::Element
{
    friend class Scheduler;

    private:
        Ec *     const          ec                  { nullptr };
        unsigned const          cpu                 { 0 };
        unsigned const          prio                { 0 };
        uint64   const          budget              { 0 };
        Atomic<uint64>          used                { 0 };
        uint64                  left                { 0 };
        uint64                  last                { 0 };

        static Slab_cache       cache;

        Sc (unsigned, Ec *, unsigned, uint32);

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
        NODISCARD
        static Sc *create (unsigned c, Ec *e, unsigned p, uint32 b)
        {
            return new Sc (c, e, p, b);
        }

        void destroy() { delete this; }

        ALWAYS_INLINE
        inline auto get_ec() const { return ec; }

        ALWAYS_INLINE
        inline uint64 get_used() const { return used; }
};

class Scheduler
{
    public:
        static constexpr unsigned priorities = 128;

        static void unblock (Sc *);
        static void requeue();

        ALWAYS_INLINE
        static inline bool livelock() { return ++helping >= 100; }

        ALWAYS_INLINE
        static inline auto get_current() { return current; }

        ALWAYS_INLINE
        static inline void set_current (Sc *s) { current = s; }

        NORETURN
        static void schedule (bool = false);

    private:
        // Ready queue
        class Ready
        {
            private:
                Queue<Sc>   queue[priorities];
                unsigned    prio_top { 0 };

            public:
                inline void enqueue (Sc *, uint64);
                inline auto dequeue (uint64);
        };

        // Release queue
        class Release
        {
            private:
                Queue<Sc>   queue;
                Spinlock    lock;

            public:
                inline void enqueue (Sc *);
                inline auto dequeue();
        };

        static Ready        ready       CPULOCAL;
        static Release      release     CPULOCAL;
        static unsigned     helping     CPULOCAL;
        static Sc *         current     CPULOCAL;
};
