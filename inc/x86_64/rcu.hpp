/*
 * Read-Copy Update (RCU)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "compiler.hpp"
#include "macros.hpp"
#include "types.hpp"

class Rcu_elem
{
    public:
        Rcu_elem *next;
        void (*func)(Rcu_elem *);

        ALWAYS_INLINE
        explicit Rcu_elem (void (*f)(Rcu_elem *)) : next (nullptr), func (f) {}
};

class Rcu_list
{
    public:
        Rcu_elem *  head;
        Rcu_elem ** tail;

        ALWAYS_INLINE
        explicit Rcu_list() { clear(); }

        ALWAYS_INLINE
        inline void clear() { head = nullptr; tail = &head; }

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
            e->next = nullptr;
           *tail = e;
            tail = &e->next;
        }
};

class Rcu
{
    private:
        static uintptr_t    count;
        static uintptr_t    state;

        static uintptr_t    l_batch CPULOCAL;
        static uintptr_t    c_batch CPULOCAL;

        static Rcu_list     next    CPULOCAL;
        static Rcu_list     curr    CPULOCAL;
        static Rcu_list     done    CPULOCAL;

        enum State
        {
            RCU_CMP = BIT (0),
            RCU_PND = BIT (1),
        };

        ALWAYS_INLINE
        static inline uintptr_t batch() { return state >> 2; }

        ALWAYS_INLINE
        static inline bool complete (uintptr_t b) { return static_cast<signed long>((state & ~RCU_PND) - (b << 2)) > 0; }

        static void start_batch (State);
        static void invoke_batch();

    public:
        ALWAYS_INLINE
        static inline void call (Rcu_elem *e) { next.enqueue (e); }

        static void quiet();
        static void update();
};
