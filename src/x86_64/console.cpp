/*
 * Generic Console
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

#include "console.hpp"
#include "lock_guard.hpp"
#include "lowlevel.hpp"

void Console::print_num (uint64 val, unsigned base, unsigned width, unsigned flags)
{
    bool neg = false;

    if (flags & Flags::SIGNED && static_cast<signed long long>(val) < 0) {
        neg = true;
        val = -val;
    }

    char buffer[24], *ptr = buffer + sizeof buffer;

    do {
        *--ptr = "0123456789abcdef"[val % base];
    } while (val /= base);

    if (neg)
        *--ptr = '-';

    unsigned long count = buffer + sizeof buffer - ptr;
    unsigned long n = count + (flags & Flags::ALT_FORM ? 2 : 0);

    if (flags & Flags::ZERO_PAD) {
        if (flags & Flags::ALT_FORM) {
            putc ('0');
            putc ('x');
        }
        while (n++ < width)
            putc ('0');
    } else {
        while (n++ < width)
            putc (' ');
        if (flags & Flags::ALT_FORM) {
            putc ('0');
            putc ('x');
        }
    }

    while (count--)
        putc (*ptr++);
}

void Console::print_str (char const *s, unsigned width, unsigned precs)
{
    if (EXPECT_FALSE (!s))
        return;

    unsigned n;

    for (n = 0; *s && precs--; n++)
        putc (*s++);

    while (n++ < width)
        putc (' ');
}

void Console::vprintf (char const *format, va_list args)
{
    while (*format) {

        if (EXPECT_TRUE (*format != '%')) {
            putc (*format++);
            continue;
        }

        auto mode = Mode::FLAGS;
        unsigned flags = 0, width = 0, precs = 0, len = 0;

        for (uint64 u;;) {

            switch (*++format) {

                case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                    switch (mode) {
                        case Mode::FLAGS:
                            if (*format == '0') {
                                flags |= Flags::ZERO_PAD;
                                break;
                            }
                            mode = Mode::WIDTH;
                            [[fallthrough]];
                        case Mode::WIDTH: width = width * 10 + *format - '0'; break;
                        case Mode::PRECS: precs = precs * 10 + *format - '0'; break;
                    }
                    continue;

                case '#':
                    if (mode == Mode::FLAGS)
                        flags |= Flags::ALT_FORM;
                    continue;

                case '*':
                    width = va_arg (args, int);
                    mode = Mode::WIDTH;
                    continue;

                case '.':
                    mode = Mode::PRECS;
                    continue;

                case 'l':
                    len++;
                    continue;

                case 'c':
                    putc (static_cast<char>(va_arg (args, int)));
                    break;

                case 's':
                    print_str (va_arg (args, char *), width, precs ? precs : ~0u);
                    break;

                case 'd':
                    switch (len) {
                        case 0:  u = va_arg (args, int);        break;
                        case 1:  u = va_arg (args, long);       break;
                        default: u = va_arg (args, long long);  break;
                    }
                    print_num (u, 10, width, flags | Flags::SIGNED);
                    break;

                case 'u':
                case 'x':
                    switch (len) {
                        case 0:  u = va_arg (args, unsigned int);        break;
                        case 1:  u = va_arg (args, unsigned long);       break;
                        default: u = va_arg (args, unsigned long long);  break;
                    }
                    print_num (u, *format == 'x' ? 16 : 10, width, flags);
                    break;

                case 'p':
                    print_num (reinterpret_cast<uintptr_t>(va_arg (args, void *)), 16, width, Flags::ALT_FORM);
                    break;

                case 0:
                    format--;
                    [[fallthrough]];

                default:
                    putc (*format);
                    break;
            }

            format++;

            break;
        }
    }

    putc ('\r');
    putc ('\n');
}

void Console::print (char const *format, ...)
{
    Lock_guard <Spinlock> guard (lock);

    va_list args;
    va_start (args, format);
    vprintf (format, args);
    va_end (args);
}

void Console::panic (char const *format, ...)
{
    {   Lock_guard <Spinlock> guard (lock);

        va_list args;
        va_start (args, format);
        vprintf (format, args);
        va_end (args);
    }

    shutdown();
}
