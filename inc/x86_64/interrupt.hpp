/*
 * Interrupt Handling
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "refcnt.hpp"
#include "smmu.hpp"
#include "status.hpp"
#include "vectors.hpp"

class Sm;

class Interrupt final
{
    private:
        static Refptr<Sm> sm_table[NUM_GSI] CPULOCAL;

        static void rke_handler();

        static void handle_ipi (unsigned);
        static void handle_lvt (unsigned);
        static void handle_gsi (unsigned);

    public:
        static        constexpr gsi_t vec_max { sizeof (sm_table) / sizeof (*sm_table) - 1 };
        static        constexpr gsi_t gsi_max { BIT (Smmu::Entry_irt::order_e) - 1 };
        static inline constinit gsi_t gsi_pin { 0 };

        enum Request
        {
            RRQ,
            RKE,
        };

        static void *get_ptr (iid_t iid) { return Smmu::Grp::lookup_irt (iid); }

        static void setup();

        static void handler (unsigned) asm ("int_handler");

        static Status assign (Sm *, cpu_t, gsi_t, pci_t, uint8_t, uintptr_t &, uintptr_t &);
        static void deactivate (Sm *);

        static void send_cpu (Request, cpu_t);
        static void send_exc (Request);
};
