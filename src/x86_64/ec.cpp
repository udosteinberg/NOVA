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
    Status s;

    auto const ec { Ec::create (Cpu::id, idle) };
    auto const sc { Pd::nova.create_sc (s, &Space_obj::nova, Space_obj::Selector::NOVA_CPU + Cpu::id, ec, Cpu::id, 1000, 0, 0) };

    assert (ec && sc);

    current = ec;
    Scheduler::set_current (sc);
}

bool Ec::load_root (Space_hst *hst, uint64_t mod_s, uint64_t mod_e, uint64_t &img_s, uint64_t &img_e, uint64_t &msize, uint64_t &entry)
{
    assert (mod_s < mod_e);

    // Module load address must be page-aligned
    if (mod_s % PAGE_SIZE (0)) [[unlikely]]
        return false;

    // Map module into remap window
    auto const len { mod_e - mod_s };
    auto const avl { min (Hpt::page_size (Hpt::bpl), len) };
    auto const ptr { reinterpret_cast<uintptr_t>(Hptp::map (MMAP_GLB_MAP0, mod_s)) };

    // ELF header must fit, image type, PH size, PH alignment must be correct
    auto const e { view_buffer_as<Eh const> (ptr, avl, 0) };
    if (!e || !e->valid (ELF_MACHINE) || e->ph_size != sizeof (Ph) || e->ph_offset % alignof (Ph)) [[unlikely]]
        return false;

    // Program headers must fit
    auto p { view_buffer_as<Ph const> (ptr, avl, e->ph_offset, e->ph_count) };
    if (!p) [[unlikely]]
        return false;

    // Determine image entry point
    entry = e->entry;

    // Initialize image defaults
    msize = 0; img_e = 0; img_s = img_e - 1;

    for (uint64_t c { e->ph_count }, f { 0 }, v { 0 }; c--; p++) {

        // Skip segments with wrong type or no size or no permissions
        if (p->type != Ph::Type::LOAD || !p->f_size || !p->flags) [[unlikely]]
            continue;

        // Segment must have page-congruent load address and virtual address
        if (p->f_size != p->m_size || p->v_addr % PAGE_SIZE (0) != (p->f_offs + mod_s) % PAGE_SIZE (0)) [[unlikely]]
            return false;

        // Segment must appear in ascending order, sorted by file offset
        if (p->f_offs < f || p->f_offs + p->f_size < p->f_offs) [[unlikely]]
            return false;

        // Segment must appear in ascending order, sorted by virtual address
        if (p->v_addr < v || p->v_addr + p->m_size < p->v_addr) [[unlikely]]
            return false;

        // Segment must fully reside within the image
        if (p->f_offs >= len || p->f_size > len - p->f_offs) [[unlikely]]
            return false;

        // Segment must not overlap the HIP and UTCB pages
        if (p->v_addr + p->m_size > Space_hst::utcb_addr()) [[unlikely]]
            return false;

        // Determine segment permissions
        auto perm { Paging::Permissions (Paging::R  * !!(p->flags & Ph::R) |
                                         Paging::W  * !!(p->flags & Ph::W) |
                                         Paging::XU * !!(p->flags & Ph::X) |
                                         Paging::U) };

        // Determine if this is the measured segment (RX permissions, starts at 0, ends at a page boundary, covers all PHs and the entry point)
        if (!msize && (p->flags & (Ph::R | Ph::W | Ph::X)) == (Ph::R | Ph::X) && !p->f_offs && !(p->f_size % PAGE_SIZE (0)) && e->ph_offset <= p->f_size && e->ph_count * sizeof (Ph) <= p->f_size - e->ph_offset && e->entry - p->v_addr < p->f_size)
            msize = p->f_size;

        trace (TRACE_ROOT | TRACE_PARSE, "ROOT: P:%#lx => V:%#lx S:%#10lx (%#x)", p->f_offs + mod_s, p->v_addr, p->f_size, perm);

        auto phys { aligned_dn (PAGE_SIZE (0), p->f_offs + mod_s) };
        auto virt { aligned_dn (PAGE_SIZE (0), p->v_addr) };
        auto size { aligned_up (PAGE_SIZE (0), p->v_addr + p->f_size) - virt };

        // Update image boundaries
        img_s = min (img_s, phys);
        img_e = max (img_e, phys + size);

        // Map segment pages into host space
        for (unsigned o; size; size -= BITN (o), phys += BITN (o), virt += BITN (o))
            if (hst->delegate (&Space_hst::nova, phys >> PAGE_BITS, virt >> PAGE_BITS, (o = aligned_order (size, phys, virt)) - PAGE_BITS, perm, Memattr::ram()) != Status::SUCCESS) [[unlikely]]
                return false;

        // Update ascending order tracking
        f = aligned_up (PAGE_SIZE (0), p->f_offs + p->f_size);
        v = aligned_up (PAGE_SIZE (0), p->v_addr + p->m_size);
    }

    return img_e;
}

void Ec::create_root()
{
    trace (TRACE_PERF, "TIME: %lums %lums/%lums",
           Stc::ticks_to_ms (Timer::time() - Multiboot::t0),
           Stc::ticks_to_ms (Multiboot::t1 - Multiboot::t0),
           Stc::ticks_to_ms (Multiboot::t2 - Multiboot::t1));

    if (Multiboot::re <= Multiboot::rs) [[unlikely]] {
        trace (TRACE_ROOT, "ROOT: No image provided");
        return;
    }

    Status s;

    Pd::root = Pd::create (s, &Pd::nova);
    Space_obj::nova.insert (Space_obj::Selector::ROOT_PD, Capability { Pd::root, std::to_underlying (Capability::Perm_pd::DEFINED) });

    auto const obj { Pd::root->create_obj (s, &Space_obj::nova, Space_obj::Selector::ROOT_OBJ) };
    auto const hst { Pd::root->create_hst (s, &Space_obj::nova, Space_obj::Selector::ROOT_HST) };
                     Pd::root->create_pio (s, &Space_obj::nova, Space_obj::Selector::ROOT_PIO);

    if (!obj || !hst) [[unlikely]]
        return;

    obj->delegate (&Space_obj::nova, Space_obj::Selector::NOVA_OBJ, Space_obj::selectors - 1, 0, Capability::pmask);
    obj->delegate (&Space_obj::nova, Space_obj::Selector::ROOT_OBJ, Space_obj::selectors - 2, 0, Capability::pmask);
    obj->delegate (&Space_obj::nova, Space_obj::Selector::ROOT_PD,  Space_obj::selectors - 3, 0, Capability::pmask);

    uint64_t root_s, root_e, msize, entry;
    if (!load_root (hst, Multiboot::rs, Multiboot::re, root_s, root_e, msize, entry)) [[unlikely]] {
        trace (TRACE_ROOT, "ROOT: Image load failed");
        return;
    }

    auto const ec { Pd::root->create_ec (s, obj, Space_obj::selectors - 4, Cpu::id, 0, 0, Space_hst::utcb_addr(), BIT (2) | BIT (1)) };
    auto const sc { Pd::root->create_sc (s, obj, Space_obj::selectors - 5, ec, Cpu::id, 1000, Scheduler::priorities - 1, 0) };

    if (!ec || !sc) [[unlikely]]
        return;

    auto const abi { Sys_abi (ec->sys_regs()) };

    abi.p0() = Multiboot::p0;
    abi.p1() = Multiboot::p1;
    abi.p2() = Multiboot::p2;

    ec->exc_regs().ip() = entry;
    ec->exc_regs().sp() = Space_hst::info_addr();
    ec->cont = Ec_arch::ret_user_hypercall;

    auto const m { false };

    trace (TRACE_ROOT, "ROOT: Invoking entry point %#lx %s measuring %#lx bytes", entry, m ? "after" : "without", msize);

    Scheduler::unblock (sc);

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
        Scheduler::schedule (false);

    Counter::helping.inc();

    ec->activate();

    Scheduler::schedule (true);
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
 * @return      False if the EC is prohibited from using the FPU; true otherwise
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

    // Enable the FPU for the save, load, or both
    if (ec || fpowner) [[likely]]
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
