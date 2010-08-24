/*
 * Standard I/O
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
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

#include "initprio.h"
#include "lock_guard.h"
#include "spinlock.h"
#include "stdio.h"

INIT_PRIORITY (PRIO_CONSOLE)
Console_serial serial;

INIT_PRIORITY (PRIO_CONSOLE)
Console_vga screen;

INIT_PRIORITY (PRIO_CONSOLE)
Spinlock printf_lock;

void panic (char const *format, ...)
{
    va_list args;
    va_start (args, format);
    screen.set_page (1);
    screen.vprintf (format, args);      // Do not grab lock
    va_end (args);

    Cpu::shutdown();
}

void printf (char const *format, ...)
{
    Lock_guard <Spinlock> guard (printf_lock);

    va_list args;

    va_start (args, format);
    screen.vprintf (format, args);
    va_end (args);

    va_start (args, format);
    serial.vprintf (format, args);
    va_end (args);
}

extern "C" NORETURN void __cxa_pure_virtual() { UNREACHED; }
