/*
 * Queue
 *
 * Copyright (C) 2007-2009, Udo Steinberg <udo@hypervisor.org>
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
#include "sc.h"
#include "types.h"

class Queue
{
    private:
        Sc *queue;

        ALWAYS_INLINE
        inline void enqueue (Sc *s)
        {
            s->next = queue;
            queue = s;
        }

        ALWAYS_INLINE
        inline Sc *dequeue()
        {
            Sc *s = queue;

            if (EXPECT_TRUE (s))
                queue = s->next;

            return s;
        }

    public:
        ALWAYS_INLINE
        inline Queue() : queue (0) {}

        ALWAYS_INLINE
        inline void block()
        {
            enqueue (Sc::current);
        }

        ALWAYS_INLINE
        inline bool release()
        {
            Sc *sc = dequeue();

            if (EXPECT_TRUE (!sc))
                return false;

            sc->remote_enqueue();

            return true;
        }
};
