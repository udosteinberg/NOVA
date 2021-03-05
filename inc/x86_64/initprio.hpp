/*
 * Static Initialization Priorities
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

// Lower numbers indicate a higher priority
#define AFTER(X)        ((X) + 1)

#define PRIO_LIMIT      100
#define PRIO_PTAB       AFTER (PRIO_LIMIT)
#define PRIO_SLAB       AFTER (PRIO_PTAB)
#define PRIO_SPACE_OBJ  AFTER (PRIO_SLAB)
#define PRIO_CONSOLE    65533
#define PRIO_LOCAL      65534
