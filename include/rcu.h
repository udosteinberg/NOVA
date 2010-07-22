/*
 * Read-Copy Update (RCU)
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
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

class Rcu_elem
{
    public:
        Rcu_elem *next;
        void (*func)(Rcu_elem *);

        explicit Rcu_elem (void (*f)(Rcu_elem *)) : func (f) {}
};

class Rcu_list
{
    public:
        Rcu_elem *  head;
        Rcu_elem ** tail;

        explicit Rcu_list() { clear(); }

        ALWAYS_INLINE
        inline void clear() { head = 0; tail = &head; }

        ALWAYS_INLINE
        inline void append (Rcu_list *l)
        {
            *tail = l->head;
             tail = l->tail;
             l->clear();
        }

        ALWAYS_INLINE
        inline void enqueue (Rcu_elem *e)
        {
             e->next = 0;
            *tail = e;
             tail = &e->next;
        }
};

class Rcu
{
    private:
        static unsigned batch;
        static unsigned count;

        static unsigned q_batch     CPULOCAL;
        static unsigned c_batch     CPULOCAL;

        static bool     q_passed    CPULOCAL;
        static bool     q_pending   CPULOCAL;

        static Rcu_list next        CPULOCAL;
        static Rcu_list curr        CPULOCAL;
        static Rcu_list done        CPULOCAL;

    public:
        ALWAYS_INLINE
        static inline void submit (Rcu_elem *e) { next.enqueue (e); }

        ALWAYS_INLINE
        static inline void quiet() { q_passed = true; }

        static void invoke_batch();
        static void start_batch();
        static void check_quiescent_state();
        static bool process_callbacks();
};
