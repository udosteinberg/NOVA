/*
 * Host Memory Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2022 Udo Steinberg, BedRock Systems, Inc.
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

#include "pd.hpp"
#include "space_hst.hpp"
#include "space_obj.hpp"

INIT_PRIORITY (PRIO_SPACE_MEM)
ALIGNED (Kobject::alignment) Space_hst Space_hst::nova;

Space_hst *Space_hst::current { nullptr };

void Space_hst::init (unsigned cpu)
{
    if (!cpus.tas (cpu)) {
        loc[cpu].share_from (Pd::kern.loc[cpu], MMAP_CPU, MMAP_SPC);
        loc[cpu].share_from_master (LINK_ADDR, MMAP_CPU);
    }
}
