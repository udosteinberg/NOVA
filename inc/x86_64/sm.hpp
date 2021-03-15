/*
 * Semaphore
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "ec.hpp"

class Sm : public Kobject, private Queue<Ec>
{
    private:
        mword           counter;
        Spinlock        lock;

        static Slab_cache cache;

    public:
        Sm (Pd *, mword, mword = 0);

        ALWAYS_INLINE
        inline void dn (Ec *const self, bool zero, uint64 t)
        {
            {   Lock_guard <Spinlock> guard (lock);

                if (counter) {
                    counter = zero ? 0 : counter - 1;
                    return;
                }

                // The EC can no longer be activated
                self->block();

                enqueue_tail (self);
            }

            // At this point remote cores can unblock the EC

            if (self->block_sc()) {

                if (t)
                    self->set_timeout (t, this);

                Scheduler::schedule (true);
            }
        }

        ALWAYS_INLINE
        inline void up()
        {
            Ec *ec;

            {   Lock_guard <Spinlock> guard (lock);

                if (!(ec = dequeue_head())) {
                    counter++;
                    return;
                }

                // The EC can now be activated again
                ec->unblock (Ec::sys_finish<Status::SUCCESS, true>);
            }

            ec->unblock_sc();
        }

        ALWAYS_INLINE NONNULL
        inline void timeout (Ec *const ec)
        {
            {   Lock_guard <Spinlock> guard (lock);

                if (!ec->blocked())
                    return;

                dequeue (ec);

                // The EC can now be activated again
                ec->unblock (Ec::sys_finish<Status::TIMEOUT>);
            }

            ec->unblock_sc();
        }

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};
