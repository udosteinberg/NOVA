/*
 * Timeout
 *
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "assert.hpp"
#include "timeout.hpp"
#include "timer.hpp"

Timeout *Timeout::list;

void Timeout::enqueue (uint64_t t)
{
    assert (this != list);
    assert (!prev);
    assert (!next);

    time = t;

    Timeout *p { nullptr };

    for (auto n { list }; n; p = n, n = n->next)
        if (n->time >= time)
            break;

    prev = p;

    if (!p) {
        next = list;
        list = this;
        sync();
    } else {
        next = p->next;
        p->next = this;
    }

    if (next)
        next->prev = this;
}

uint64_t Timeout::dequeue()
{
    if (list == this || prev) {

        if (next)
            next->prev = prev;

        if (prev)
            prev->next = next;

        else {
            list = next;
            sync();
        }

        prev = next = nullptr;
    }

    assert (this != list);
    assert (!prev);
    assert (!next);

    return time;
}

void Timeout::check()
{
    while (list && list->time <= Timer::time()) {
        Timeout *t = list;
        t->dequeue();
        t->trigger();
    }
}

void Timeout::sync()
{
    if (list)
        Timer::set_dln (list->time);
    else
        Timer::stop();
}
