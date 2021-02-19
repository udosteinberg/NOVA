/*
 * Read-Copy Update (RCU)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
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

#include "buddy.hpp"

class Rcu
{
    public:
        struct Element
        {
            Element *rcu_next { nullptr };

            virtual void destroy() = 0;

            void rcu_submit() { elem_next.enqueue (this); }
        };

        // Free a page after the next grace period
        static void submit (void *ptr) { page_next.enqueue (ptr); }

        static void quiet();
        static void check();

    private:
        class List_elem
        {
            private:
                Element *head { nullptr };

            public:
                explicit operator bool() const { return head; }

                List_elem &operator= (List_elem &&l)
                {
                    assert (!head);

                    head = l.head;

                    l.head = nullptr;

                    return *this;
                }

                void enqueue (Element *e)
                {
                    e->rcu_next = head;
                    head = e;
                }

                void drain()
                {
                    for (Element *e { head }, *n; e; e = n) {
                        n = e->rcu_next;
                        e->destroy();
                    }

                    head = nullptr;
                }
        };

        class List_page
        {
            private:
                Buddy::Block *head { nullptr };

            public:
                explicit operator bool() const { return head; }

                List_page &operator= (List_page &&l)
                {
                    assert (!head);

                    head = l.head;

                    l.head = nullptr;

                    return *this;
                }

                /*
                 * Enqueue a used page at the head of the list
                 *
                 * @param ptr       Pointer to virtual memory region
                 */
                void enqueue (void *ptr)
                {
                    auto const idx { Buddy::page_to_index (reinterpret_cast<uintptr_t>(ptr)) };

                    // Ensure memory is within allocator range
                    assert (Buddy::valid (idx));

                    auto const b { Buddy::index_to_block (idx) };

                    // Ensure block is not linked elsewhere
                    assert (!b->get_prev() && !b->get_next());

                    b->set_next (head);
                    head = b;
                }

                /*
                 * Free all pages on the list and empty it
                 */
                void drain()
                {
                    for (Buddy::Block *b { head }, *n; b; b = n) {
                        n = static_cast<Buddy::Block *>(b->get_next());
                        b->set_next (nullptr);
                        Buddy::coalesce (b);
                    }

                    head = nullptr;
                }
        };

        using Epoch = unsigned long;

        enum State
        {
            COMPLETED = BIT (0),            // Completed epoch E
            REQUESTED = BIT (1),            // Requested epoch E+1
            FULL      = REQUESTED | COMPLETED,
        };

        // Epoch and state tracking
        static inline constinit Atomic<Epoch, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST> epoch { State::COMPLETED };

        // Number of CPUs that still need to pass through a quiescent state in epoch E
        static inline constinit Atomic<cpu_t, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST> count { 0 };

        static List_elem elem_next CPULOCAL;    // Callbacks handled in a future epoch
        static List_elem elem_curr CPULOCAL;    // Callbacks handled in epoch_c
        static List_elem elem_done CPULOCAL;    // Callbacks completed in earlier epochs

        static List_page page_next CPULOCAL;    // Pages freed in a future epoch
        static List_page page_curr CPULOCAL;    // Pages freed in epoch_c
        static List_page page_done CPULOCAL;    // Pages ready to be freed

        static Epoch epoch_l CPULOCAL;          // Epoch for which quiescent state will be reported
        static Epoch epoch_c CPULOCAL;          // Epoch for which current callbacks are handled

        static void set_state (State);

        static bool complete (Epoch e, Epoch c) { return static_cast<signed long>((e & ~State::REQUESTED) - (c << 2)) > 0; }
};
