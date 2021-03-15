/*
 * Execution Context (EC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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

#include "abi.hpp"
#include "counter.hpp"
#include "ec_arch.hpp"
#include "elf.hpp"
#include "extern.hpp"
#include "fpu.hpp"
#include "hip.hpp"
#include "interrupt.hpp"
#include "multiboot.hpp"
#include "sm.hpp"
#include "space_hst.hpp"
#include "space_obj.hpp"
#include "stdio.hpp"
#include "timer.hpp"

Atomic<Ec *, __ATOMIC_RELAXED, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST> Ec::current { nullptr };
Ec *            Ec::fpowner     { nullptr };
unsigned        Ec::donations   { 0 };

// Factory: Kernel Thread
Ec *Ec::create (cpu_t c, cont_t x)
{
    // Acquire references
    Refptr<Space_obj> ref_obj { &Space_obj::nova };
    Refptr<Space_hst> ref_hst { &Space_hst::nova };
    Refptr<Space_pio> ref_pio { nullptr };

    // Failed to acquire references
    if (!ref_obj || !ref_hst) [[unlikely]]
        return nullptr;

    // Create new EC object
    auto const obj { new (Pd::nova.ec_cache) Ec_arch { ref_obj, ref_hst, ref_pio, c, x } };

    // If creation succeeded, then references must have been consumed
    assert (!obj || (!ref_obj && !ref_hst && !ref_pio));

    return obj;
}

// Factory: HST EC
Ec *Ec::create_hst (Status &s, Pd *pd, bool t, bool fpu, cpu_t cpu, uintptr_t evt, uintptr_t sp, uintptr_t hva)
{
    // Acquire references
    Refptr<Space_obj> ref_obj { pd->get_obj() };
    Refptr<Space_hst> ref_hst { pd->get_hst() };
    Refptr<Space_pio> ref_pio { pd->get_pio() };

    // Failed to acquire references
    if (!ref_obj || !ref_hst || (Ec_arch::needs_pio && !ref_pio)) [[unlikely]] {
        s = Status::ABORTED;
        return nullptr;
    }

    auto const f { fpu ? new (pd->fpu_cache) Fpu : nullptr };
    auto const u { new Utcb };
    Ec *ec;

    if ((!fpu || f) && u && (ec = new (pd->ec_cache) Ec_arch { t, f, ref_obj, ref_hst, ref_pio, cpu, evt, sp, hva, u })) [[likely]] {
        assert (!ref_obj && !ref_hst && !ref_pio);
        return ec;
    }

    delete u;
    Fpu::operator delete (f, pd->fpu_cache);

    s = Status::MEM_OBJ;

    return nullptr;
}

void Ec::destroy()
{
    auto &cache { get_pd()->ec_cache };

    this->Ec::~Ec();

    operator delete (this, cache);
}

void Ec::create_idle()
{
    auto const ec { Ec::create (Cpu::id, idle) };
    auto const sc { new Sc (nullptr, Cpu::id, ec) };

    assert (ec && sc);

    current = ec;
    Sc::current = sc;
}

void Ec::create_root()
{
    trace (TRACE_PERF, "TIME: %lums %lums/%lums",
           Stc::ticks_to_ms (Timer::time() - Multiboot::t0),
           Stc::ticks_to_ms (Multiboot::t1 - Multiboot::t0),
           Stc::ticks_to_ms (Multiboot::t2 - Multiboot::t1));

    auto const ra { Multiboot::ra };

    if (!ra) [[unlikely]] {
        trace (TRACE_ROOT, "ROOT: No image provided");
        return;
    }

    Status s;

    Pd::root = Pd::create (s, &Pd::nova);
    Space_obj::nova.insert (Space_obj::Selector::ROOT_PD, Capability { Pd::root, std::to_underlying (Capability::Perm_pd::DEFINED) });

    auto const obj { Pd::root->create_obj (s, &Space_obj::nova, Space_obj::Selector::ROOT_OBJ) };
    auto const hst { Pd::root->create_hst (s, &Space_obj::nova, Space_obj::Selector::ROOT_HST) };
                     Pd::root->create_pio (s, &Space_obj::nova, Space_obj::Selector::ROOT_PIO);

    obj->delegate (&Space_obj::nova, Space_obj::Selector::NOVA_OBJ, Space_obj::selectors - 1, 0, Capability::pmask);
    obj->delegate (&Space_obj::nova, Space_obj::Selector::ROOT_OBJ, Space_obj::selectors - 2, 0, Capability::pmask);
    obj->delegate (&Space_obj::nova, Space_obj::Selector::ROOT_PD,  Space_obj::selectors - 3, 0, Capability::pmask);

    auto info_addr { (Space_hst::selectors - 1) << PAGE_BITS };
    auto utcb_addr { (Space_hst::selectors - 2) << PAGE_BITS };

    auto const ec { Pd::root->create_ec (s, obj, Space_obj::selectors - 4, Cpu::id, 0, 0, utcb_addr, BIT (2) | BIT (1)) };
    auto const sc { new Sc (Pd::root, NUM_EXC + 2, ec, Cpu::id, Sc::default_prio, Sc::default_quantum) };

    if (!ec || !sc) [[unlikely]]
        return;

    auto const e { static_cast<Eh const *>(Hptp::map (MMAP_GLB_MAP0, ra)) };

    if (!e->valid (ELF_MACHINE)) [[unlikely]] {
        trace (TRACE_ROOT, "ROOT: Invalid image provided");
        return;
    }

    auto const abi { Sys_abi (ec->sys_regs()) };

    abi.p0() = Multiboot::p0;
    abi.p1() = Multiboot::p1;
    abi.p2() = Multiboot::p2;

    ec->cont = Ec_arch::ret_user_hypercall;
    ec->exc_regs().ip() = e->entry;
    ec->exc_regs().sp() = info_addr;

    uint64_t root_e { 0 }, root_s { root_e - 1 };

    auto p { static_cast<Ph const *>(Hptp::map (MMAP_GLB_MAP1, ra + e->ph_offset)) };

    for (auto c { e->ph_count }; c--; p++) {

        if (p->type != 1 || !p->f_size || !p->flags)
            continue;

        if (p->f_size != p->m_size || p->v_addr % PAGE_SIZE (0) != (p->f_offs + ra) % PAGE_SIZE (0))
            return;

        auto perm { Paging::Permissions (Paging::R  * !!(p->flags & BIT (2)) |
                                         Paging::W  * !!(p->flags & BIT (1)) |
                                         Paging::XU * !!(p->flags & BIT (0)) |
                                         Paging::U) };

        trace (TRACE_ROOT | TRACE_PARSE, "ROOT: P:%#lx => V:%#lx S:%#10lx (%#x)", p->f_offs + ra, p->v_addr, p->f_size, perm);

        auto phys { aligned_dn (PAGE_SIZE (0), p->f_offs + ra) };
        auto virt { aligned_dn (PAGE_SIZE (0), p->v_addr) };
        auto size { aligned_up (PAGE_SIZE (0), p->v_addr + p->f_size) - virt };

        root_s = min (root_s, phys);
        root_e = max (root_e, phys + size);

        for (unsigned o; size; size -= BITN (o), phys += BITN (o), virt += BITN (o))
            if (hst->delegate (&Space_hst::nova, phys >> PAGE_BITS, virt >> PAGE_BITS, (o = aligned_order (size, phys, virt)) - PAGE_BITS, perm, Memattr::ram()) != Status::SUCCESS) {
                trace (TRACE_ROOT, "ROOT: Cannot load image");
                return;
            }
    }

    sc->remote_enqueue();

    Console::flush();
}

void Ec::activate()
{
    auto ec { this };

    for (donations = 0; ec->callee; ec = ec->callee)
        donations++;

    // Fast path: EC is not blocked (and has no chance to block).
    // Slow path: EC may be unblocked from a remote core anytime.
    if (!ec->blocked() || !ec->block_sc()) [[likely]]
        static_cast<Ec_arch *>(ec)->make_current();
}

void Ec::help (Ec *ec, cont_t c)
{
    if (ec->cont == dead) [[unlikely]]
        return;

    cont = c;

    // Preempt long helping chains, including livelocks
    Cpu::preemption_point();
    if (Cpu::hazard & Hazard::SCHED) [[unlikely]]
        Sc::schedule (false);

    Counter::helping.inc();

    ec->activate();

    Sc::schedule (true);
}

void Ec::idle (Ec *const self)
{
    trace (TRACE_CONT, "EC:%p %s", static_cast<void *>(self), __func__);

    for (;;) {

        auto hzd { Cpu::hazard & (Hazard::RCU | Hazard::SLEEP | Hazard::SCHED) };
        if (hzd) [[unlikely]]
            self->handle_hazard (hzd, idle);

        Cpu::halt();
    }
}

void Ec::kill (char const *reason)
{
    trace (TRACE_KILL, "Killed EC:%p (%s)", static_cast<void *>(this), reason);

    auto const ec { caller };

    if (ec)
        ec->cont = ec->cont == Ec_arch::ret_user_hypercall ? sys_finish<Status::ABORTED> : dead;

    reply (dead);
}

/*
 * Switch FPU ownership
 *
 * @param ec    Prospective new owner (or nullptr to unassign FPU)
 * @return      True if FPU ownership was changed, false otherwise
 */
bool Ec::switch_fpu (Ec *ec)
{
    // We want to assign the FPU to an EC
    if (ec) [[likely]] {

        // The EC must not be the FPU owner
        assert (fpowner != ec);

        // The FPU must be disabled
        assert (!(Cpu::hazard & Hazard::FPU));

        // The EC is not eligible to use the FPU
        if (!ec->fpu) [[unlikely]]
            return false;
    }

    Fpu::enable();

    // Save state of previous owner
    if (fpowner) [[likely]]
        fpowner->fpu_save();

    fpowner = ec;

    // Load state of new owner
    if (fpowner) [[likely]]
        fpowner->fpu_load();

    return true;
}
