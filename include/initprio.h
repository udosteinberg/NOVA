/*
 * Static Initialization Priorities
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#define AFTER(X)        (X + 1)

#define PRIO_GLOBAL     100
#define PRIO_CONSOLE    AFTER (PRIO_GLOBAL)
#define PRIO_BUDDY      AFTER (PRIO_CONSOLE)
#define PRIO_SLAB       AFTER (PRIO_BUDDY)
#define PRIO_LOCAL      0xfffe
