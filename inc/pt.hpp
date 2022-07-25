/*
 * Portal (PT)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2023 Udo Steinberg, BedRock Systems, Inc.
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
        Refptr<Ec> const    ec;
        uintptr_t  const    ip;
        Atomic<uintptr_t>   id      { 0 };
        Atomic<Mtd_arch>    mtd     { Mtd_arch { 0 } };

        static Slab_cache   cache;

        Pt (Refptr<Ec>&, uintptr_t);

    public:
        [[nodiscard]] static Pt *create (Status &s, Ec *ec, uintptr_t ip)
        {
            // Acquire reference
            Refptr<Ec> ref_ec { ec };

            // Failed to acquire reference
            if (EXPECT_FALSE (!ref_ec))
                s = Status::ABORTED;

            else {

                auto const pt { new (cache) Pt { ref_ec, ip } };

                // If we created pt, then reference must have been consumed
                assert (!pt || !ref_ec);

                if (EXPECT_TRUE (pt))
                    return pt;

                s = Status::MEM_OBJ;
            }

            return nullptr;
        }

        void destroy() { operator delete (this, cache); }

        Ec *get_ec() const { return ec; }

        uintptr_t get_ip() const { return ip; }

        uintptr_t get_id() const { return id; }

        Mtd_arch get_mtd() const { return mtd; }

        void set_id (uintptr_t i) { id = i; }

        void set_mtd (Mtd_arch m) { mtd = m; }
};
