/*
 * Read-Copy Update (RCU)
 *
 * Copyright (C) 2008-2010, Udo Steinberg <udo@hypervisor.org>
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
#include "spinlock.h"
#include "types.h"

class Rcu_elem
{
    friend class Rcu;

    private:
        Rcu_elem *next;
        void (*func)(Rcu_elem *);
};

class Rcu
{
    private:
        static Spinlock g_lock;
        static unsigned g_cpumask;
        static unsigned g_current;
        static unsigned g_completed;
        static bool     g_next_pending;

        static unsigned q_batch     CPULOCAL;
        static bool     q_passed    CPULOCAL;
        static bool     q_pending   CPULOCAL;

        static unsigned     batch   CPULOCAL;
        static Rcu_elem *   list_c  CPULOCAL;
        static Rcu_elem **  tail_c  CPULOCAL;
        static Rcu_elem *   list_n  CPULOCAL;
        static Rcu_elem **  tail_n  CPULOCAL;
        static Rcu_elem *   list_d  CPULOCAL;
        static Rcu_elem **  tail_d  CPULOCAL;

    public:
        static void init();
        static void call (Rcu_elem *, void (*)(Rcu_elem *));
        static bool pending();
        static bool process_callbacks();
        static void start_batch();
        static void check_quiescent_state();
        static void quiet();
        static bool do_batch();

        static void qstate() { q_passed = true; }
};
