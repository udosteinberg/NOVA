/*
 * Scheduler
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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

#include "queue.hpp"
#include "spinlock.hpp"

class Sc;

class Scheduler final
{
    public:
        static constexpr auto priorities { 128 };

        static void unblock (Sc *);
        static void requeue();

        static auto get_current() { return current; }

        static void set_current (Sc *s) { current = s; }

        [[noreturn]] static void schedule (bool = false);

    private:
        // Ready queue
        class Ready final
        {
            private:
                Queue<Sc>   queue[priorities];
                unsigned    prio_top { 0 };

            public:
                void enqueue (Sc *, uint64_t);
                auto dequeue (uint64_t);
        };

        // Release queue
        class Release final
        {
            private:
                Queue<Sc>   queue;
                Spinlock    lock;

            public:
                void enqueue (Sc *);
                auto dequeue();
        };

        static Ready        ready       CPULOCAL;
        static Release      release     CPULOCAL;
        static Sc *         current     CPULOCAL;
};
