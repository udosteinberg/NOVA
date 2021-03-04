/*
 * Circular Queue
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2025 Udo Steinberg, BlueRock Security, Inc.
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

#include "assert.hpp"
#include "compiler.hpp"

template<typename T> class Queue
{
    public:
        class Element
        {
            private:
                Element *prev { nullptr };
                Element *next { nullptr };

            public:
                auto get_prev() const { return prev; }
                auto get_next() const { return next; }

                void set_prev (Element *e) { prev = e; }
                void set_next (Element *e) { next = e; }
        };

        /*
         * Determine if this queue is empty
         *
         * @return      True if the queue is empty, false otherwise
         */
        ALWAYS_INLINE
        inline bool empty() const { return !head; }

        /*
         * Enqueue T* element into this queue
         *
         * @param e     Element to enqueue
         * @param h     Enqueue as head (true) or tail (false)
         * @return      True if the queue was empty, false otherwise
         */
        ALWAYS_INLINE NONNULL
        inline bool enqueue (T *e, bool h)
        {
            // Sanity check: Element must be dequeued
            assert (!e->get_prev() && !e->get_next());

            if (!head) {
                e->set_prev (e);
                e->set_next (e);
                head = e;
                return true;
            }

            auto const p { head->get_prev() };
            auto const n { head };

            e->set_prev (p);
            e->set_next (n);
            n->set_prev (e);
            p->set_next (e);

            if (h)
                head = e;

            return false;
        }

        ALWAYS_INLINE NONNULL
        inline auto enqueue_head (T *e) { return enqueue (e, true); }

        ALWAYS_INLINE NONNULL
        inline auto enqueue_tail (T *e) { return enqueue (e, false); }

        /*
         * Dequeue T* element from this queue
         *
         * @param e     Element to dequeue
         */
        ALWAYS_INLINE NONNULL
        inline void dequeue (T *e)
        {
            // Sanity check: Element must be enqueued
            assert (e->get_prev() && e->get_next());

            auto const p { e->get_prev() };
            auto const n { e->get_next() };

            if (e == n)
                head = nullptr;

            else {
                n->set_prev (p);
                p->set_next (n);
                if (e == head)
                    head = n;
            }

            e->set_prev (nullptr);
            e->set_next (nullptr);
        }

        /*
         * Dequeue T* head element from this queue
         *
         * @return      Element that was dequeued or nullptr
         */
        ALWAYS_INLINE
        inline auto dequeue_head()
        {
            auto const h { static_cast<T *>(head) };

            if (h)
                dequeue (h);

            return h;
        }

    private:
        Element *head { nullptr };
};
