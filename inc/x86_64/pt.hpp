/*
 * Portal (PT)
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

#include "ec.hpp"
#include "mtd_arch.hpp"

class Pt final : public Kobject
{
    private:
        Refptr<Ec> const ec;    // Bound EC (also implies Owner PD)
        uintptr_t  const ip;    // Entry IP

        /*
         * Memory Ordering
         *
         * ID/MTD changes are observable as follows:
         * - Ambient CPU: after ctrl_pt returned
         * - Remote CPUs: after external ACQUIRE/RELEASE synchronization with ambient CPU, denoting that ctrl_pt returned
         */
        Atomic<uintptr_t, __ATOMIC_RELAXED, __ATOMIC_RELAXED, __ATOMIC_RELAXED> id  { 0 };
        Atomic<Mtd_arch,  __ATOMIC_RELAXED, __ATOMIC_RELAXED, __ATOMIC_RELAXED> mtd { Mtd_arch { 0 } };

        explicit Pt (Refptr<Ec> &, uintptr_t);

        void collect() override final;

    public:
        [[nodiscard]] static Pt *create (Status &, Ec *, uintptr_t);

        void destroy() override final;

        Ec *get_ec() const { return ec; }

        uintptr_t get_ip() const { return ip; }

        uintptr_t get_id() const { return id; }

        auto get_mtd() const { return Mtd_arch { mtd.load() }; }

        void set_id (uintptr_t i) { id = i; }

        void set_mtd (Mtd_arch m) { mtd.store (m); }
};
