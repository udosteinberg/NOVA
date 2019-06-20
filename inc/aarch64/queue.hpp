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
                Element *next { nullptr };
                Element *prev { nullptr };

            public:
                bool queued() const { return next; }
        };

        /*
         * Return head element of queue
         *
         * @return      Head element or nullptr
         */
        ALWAYS_INLINE
        inline auto head() const { return static_cast<T *>(headptr); }

        /*
         * Enqueue element into queue
         *
         * @param e     Element to enqueue
         * @param h     Enqueue as head (true) or tail (false)
         * @return      True if the queue was empty, false otherwise
         */
        ALWAYS_INLINE NONNULL
        inline bool enqueue (T *e, bool h = false)
        {
            assert (!e->queued());

            if (!headptr) {
                headptr = e->prev = e->next = e;
                return true;
            }

            e->next = headptr;
            e->prev = headptr->prev;
            e->next->prev = e->prev->next = e;

            if (h)
                headptr = e;

            return false;
        }

        /*
         * Dequeue element from queue
         *
         * @param e     Element to dequeue
         */
        ALWAYS_INLINE NONNULL
        inline void dequeue (T *e)
        {
            assert (e->queued());

            if (e == e->next)
                headptr = nullptr;

            else {
                e->next->prev = e->prev;
                e->prev->next = e->next;
                if (e == headptr)
                    headptr = e->next;
            }

            e->next = e->prev = nullptr;
        }

    private:
        Element *headptr { nullptr };
};
