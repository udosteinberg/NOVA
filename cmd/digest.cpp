/*
 * NOVA Integrity Measurement Tool for Intel TXT
 *
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

#include <cstdint>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

struct Mh final
{
    uint64_t uuid[2];
    uint32_t size, version, entry, first, mle_start, mle_end, mle_caps, cmd_start, cmd_end;

    [[nodiscard]] bool valid() const
    {
        return uuid[0] == 0x74a7476f9082ac5a && uuid[1] == 0x42b651cba2555c0f && size == 52 && version == 0x20003;
    }
};

struct Eh final
{
    uint32_t ei_magic;
    uint8_t  ei_class, ei_data, ei_version, ei_osabi, ei_abiversion, ei_pad[7];
    uint16_t type, machine;
    uint32_t version;
    uint64_t entry, ph_offset, sh_offset;
    uint32_t flags;
    uint16_t eh_size, ph_size, ph_count, sh_size, sh_count, strtab;

    [[nodiscard]] bool valid() const
    {
        return ei_magic == 0x464c457f && ei_class == 2 && ei_data == 1 && ei_version == 1 && type == 2 && machine == 0x3e;
    }
};

struct Ph final
{
    uint32_t type, flags;
    uint64_t f_offs, v_addr, p_addr, f_size, m_size, align;
};

int main (int argc, char **argv)
{
    struct stat st;

    if (argc < 2) {
        std::cerr << "You must specify a file.\n";
        return 1;
    }

    if (fstat (fileno (stdout), &st) == -1) {
        perror ("fstat");
        return 2;
    }

    if (!S_ISFIFO (st.st_mode)) {
        std::cerr << "You must pipe the output through a hash program.\n";
        return 3;
    }

    auto const fd { open (argv[1], O_RDONLY) };

    if (fd == -1) {
        perror ("open");
        return 4;
    }

    if (fstat (fd, &st) == -1) {
        perror ("fstat");
        return 5;
    }

    auto const ptr { mmap (nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0) };

    if (ptr == MAP_FAILED) {
        perror ("mmap");
        return 6;
    }

    close (fd);

    auto const addr { reinterpret_cast<uintptr_t>(ptr) };

    uint64_t size { 0 };

    for (auto x { addr }; x < addr + st.st_size; x += sizeof (uint64_t)) {
        auto const mh { reinterpret_cast<Mh *>(x) };
        if (mh->valid())
            size = mh->mle_end - mh->mle_start;
    }

    if (!size) {
        std::cerr << "File is not an MLE.\n";
        return 7;
    }

    auto const eh { reinterpret_cast<Eh const *>(addr) };

    if (!eh->valid()) {
        std::cerr << "File is not an ELF.\n";
        return 8;
    }

    auto ph { reinterpret_cast<Ph const *>(addr + eh->ph_offset) };

    for (auto c { eh->ph_count }; size && c--; ph++) {

        if (ph->type != 1 || ph->f_size == 0)
            continue;

        auto const s { std::min (size, ph->f_size) };

        if (fwrite (reinterpret_cast<void const *>(addr + ph->f_offs), s, 1, stdout) != 1) {
            perror ("fwrite");
            return 9;
        }

        size -= s;
    }

    return 0;
}
