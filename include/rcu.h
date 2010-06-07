/*
 * Read-Copy Update (RCU)
 *
 * Author: Udo Steinberg <udo@hypervisor.org>
 * TU Dresden, Operating Systems Group
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

        static Rcu_elem *list_n     CPULOCAL;
        static Rcu_elem *list_c     CPULOCAL;
        static Rcu_elem *list_d     CPULOCAL;

    public:
        static void quiet() { q_passed = true; }

        static void invoke_batch();
        static void start_batch();
        static void check_quiescent_state();
        static bool process_callbacks();
};
