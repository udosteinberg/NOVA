/*
 * Scheduling Context (SC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2019-2024 Udo Steinberg, BlueRock Security, Inc.
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

class Sc final : public Kobject, public Queue<Sc>::Element
{
    friend class Scheduler;

    private:
        Refptr<Ec> const    ec;
        uint64_t   const    budget;
        cpu_t      const    cpu;
        cos_t      const    cos;
        uint8_t    const    prio;
        Atomic<uint64_t>    used    { 0 };
        uint64_t            left    { 0 };
        uint64_t            last    { 0 };

        static Slab_cache   cache;

        Sc (Refptr<Ec>&, cpu_t, uint16_t, uint8_t, cos_t);

        void collect() override final
        {
            trace (TRACE_DESTROY, "KOBJ: SC %p collected", static_cast<void *>(this));
        }

    public:
        [[nodiscard]] static Sc *create (Status &s, Ec *ec, cpu_t cpu, uint16_t budget, uint8_t prio, cos_t cos)
        {
            // Acquire reference
            Refptr<Ec> ref_ec { ec };

            // Failed to acquire reference
            if (!ref_ec) [[unlikely]]
                s = Status::ABORTED;

            else {

                auto const sc { new (cache) Sc { ref_ec, cpu, budget, prio, cos } };

                // If we created sc, then reference must have been consumed
                assert (!sc || !ref_ec);

                if (sc) [[likely]]
                    return sc;

                s = Status::MEM_OBJ;
            }

            return nullptr;
        }

        void destroy()
        {
            this->~Sc();

            operator delete (this, cache);
        }

        Ec *get_ec() const { return ec; }

        uint64_t get_used() const { return used; }
};
