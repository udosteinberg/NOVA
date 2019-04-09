/*
 * Read-Copy Update (RCU)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2024 Udo Steinberg, BedRock Systems, Inc.
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
#include "macros.hpp"
#include "types.hpp"

class Rcu_elem
{
    public:
        Rcu_elem *next;
        void (*func)(Rcu_elem *);

        Rcu_elem (void (*f)(Rcu_elem *)) : next { nullptr }, func { f } {}
};

class Rcu
{
    private:
        class List
        {
            public:
                Rcu_elem *  head;
                Rcu_elem ** tail;

                List() { clear(); }

                void clear() { head = nullptr; tail = &head; }

                void append (List *l)
                {
                   *tail = l->head;
                    tail = l->tail;
                    l->clear();
                }

                void enqueue (Rcu_elem *e)
                {
                    e->next = nullptr;
                   *tail = e;
                    tail = &e->next;
                }
        };

        using Epoch = unsigned long;

        enum State
        {
            COMPLETED = BIT (0),
            REQUESTED = BIT (1),
            FULL      = REQUESTED | COMPLETED,
        };

        // Epoch and state tracking
        static inline Atomic<Epoch, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST> epoch { State::COMPLETED };

        // Number of CPUs that still need to pass through a quiescent state in epoch E
        static inline Atomic<cpu_t, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST> count { 0 };

        static List     next    CPULOCAL;
        static List     curr    CPULOCAL;
        static List     done    CPULOCAL;

        static Epoch    epoch_l CPULOCAL;
        static Epoch    epoch_c CPULOCAL;

        static void set_state (State);

        static bool complete (Epoch e, Epoch c) { return static_cast<signed long>((e & ~State::REQUESTED) - (c << 2)) > 0; }

        static void handle_callbacks();

    public:
        static void quiet();
        static void check();

        static void submit (Rcu_elem *e) { next.enqueue (e); }
};
