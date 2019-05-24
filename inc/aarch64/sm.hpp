/*
 * Semaphore (SM)
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

#include "ec.hpp"

class Sm : public Kobject, private Queue<Ec>
{
    private:
        uint64          counter { 0 };
        Spinlock        lock;

        static Slab_cache cache;

        Sm (uint64, unsigned);

        NODISCARD
        static inline void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }

        static inline void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                cache.free (ptr);
        }

    public:
        unsigned const id;

        NODISCARD
        static Sm *create (uint64 c, unsigned i = ~0U)
        {
            return new Sm (c, i);
        }

        void destroy() { delete this; }

        ALWAYS_INLINE
        inline void dn (Ec *const ec, bool zero, uint64 t)
        {
            {   Lock_guard <Spinlock> guard (lock);

                if (counter) {
                    counter = zero ? 0 : counter - 1;
                    return;
                }

                enqueue (ec);
            }

            if (ec->block_sc()) {

                if (t)
                    ec->set_timeout (t, this);

                Sc::schedule (true);
            }
        }

        ALWAYS_INLINE
        inline bool up()
        {
            Ec *ec;

            {   Lock_guard <Spinlock> guard (lock);

                if (!(ec = head())) {

                    if (counter == ~0ULL)
                        return false;

                    counter++;

                    return true;
                }

                dequeue (ec);
            }

            // ec->release (Ec::sys_finish<Sys_regs::SUCCESS, true>);

            return true;
        }

        ALWAYS_INLINE NONNULL
        inline void timeout (Ec *ec)
        {
            {   Lock_guard <Spinlock> guard (lock);

                if (!ec->queued())
                    return;

                dequeue (ec);
            }

            // ec->release (Ec::sys_finish<Sys_regs::TIMEOUT>);
        }
};
