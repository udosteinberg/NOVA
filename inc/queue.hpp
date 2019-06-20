/*
 * Queue
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

#include "assert.hpp"
#include "compiler.hpp"

template <typename T>
class Queue
{
    public:
        class Element
        {
            friend class Queue<T>;

            private:
                Queue   *qptr { nullptr };
                Element *next { nullptr };
                Element *prev { nullptr };

            public:
                /*
                 * Determine if this element is queued
                 *
                 * @return      True if the element is queued, false otherwise
                 */
                ALWAYS_INLINE
                inline bool queued() const { return qptr; }
        };

        /*
         * Determine if this queue is empty
         *
         * @return      True if the queue is empty, false otherwise
         */
        ALWAYS_INLINE
        inline bool empty() const { return !head; }

        /*
         * Enqueue element into this queue
         *
         * @param e     Element to enqueue
         * @param h     Enqueue as head (true) or tail (false)
         * @return      True if the queue was empty, false otherwise
         */
        ALWAYS_INLINE NONNULL
        inline bool enqueue (T *e, bool h)
        {
            assert (!e->queued());

            e->qptr = this;

            if (!head) {
                head = e->prev = e->next = e;
                return true;
            }

            e->next = head;
            e->prev = head->prev;
            e->next->prev = e->prev->next = e;

            if (h)
                head = e;

            return false;
        }

        ALWAYS_INLINE NONNULL
        inline auto enqueue_head (T *e) { return enqueue (e, true); }

        ALWAYS_INLINE NONNULL
        inline auto enqueue_tail (T *e) { return enqueue (e, false); }

        /*
         * Dequeue element from this queue
         *
         * @param e     Element to dequeue
         */
        ALWAYS_INLINE NONNULL
        inline void dequeue (T *e)
        {
            assert (e->queued());

            e->qptr = nullptr;

            if (e == e->next)
                head = nullptr;

            else {
                e->next->prev = e->prev;
                e->prev->next = e->next;
                if (e == head)
                    head = e->next;
            }

            e->next = e->prev = nullptr;
        }

        /*
         * Dequeue head element from this queue
         *
         * @return      Element that was dequeued or nullptr
         */
        ALWAYS_INLINE
        inline auto dequeue_head()
        {
            auto h = static_cast<T *>(head);

            if (h)
                dequeue (h);

            return h;
        }

    private:
        Element *head { nullptr };
};
