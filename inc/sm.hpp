/*
 * Semaphore (SM)
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

#include "ec.hpp"

class Sm final : public Kobject, private Queue<Ec>
{
    private:
        uint64_t        cnt;
        void *    const ptr;
        iid_t     const iid;
        Spinlock        lock;

        static Slab_cache cache;

        explicit Sm (uintptr_t, void *);

        void collect() override final
        {
            trace (TRACE_DESTROY, "KOBJ: SM %p collected", static_cast<void *>(this));
        }

    public:
        auto get_ptr() const { assert (subtype == Kobject::Subtype::SM_INT); return ptr; }
        auto get_iid() const { assert (subtype == Kobject::Subtype::SM_INT); return iid; }

        [[nodiscard]] static Sm *create (Status &s, uintptr_t v, void *p)
        {
            auto const sm { new (cache) Sm (v, p) };

            if (!sm) [[unlikely]]
                s = Status::MEM_OBJ;

            return sm;
        }

        void destroy()
        {
            this->~Sm();

            operator delete (this, cache);
        }

        void dn (Ec *const self, bool zero, uint64_t t)
        {
            {   Lock_guard <Spinlock> guard { lock };

                if (cnt) {
                    cnt = zero ? 0 : cnt - 1;
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

        bool up()
        {
            Ec *ec;

            {   Lock_guard <Spinlock> guard { lock };

                if (!(ec = dequeue_head())) {

                    if (cnt == ~0ULL) [[unlikely]]
                        return false;

                    cnt++;

                    return true;
                }

                // The EC can now be activated again
                ec->unblock (Ec::sys_finish<Status::SUCCESS, true>, false);
            }

            ec->unblock_sc();

            return true;
        }

        NONNULL
        void timeout (Ec *const ec)
        {
            {   Lock_guard <Spinlock> guard { lock };

                if (!ec->blocked())
                    return;

                dequeue (ec);

                // The EC can now be activated again
                ec->unblock (Ec::sys_finish<Status::TIMEOUT>, true);
            }

            ec->unblock_sc();
        }
};
