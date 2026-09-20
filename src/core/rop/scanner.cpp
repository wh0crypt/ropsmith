//===----------------------------------------------------------------------===//
//
// Part of the ROPsmith Project.
// See LICENSE for license information.
//
//===----------------------------------------------------------------------===//
//!
//! \file core/rop/scanner.cpp
//! Implementation of functions for scanning ELF binaries and detecting ROP
//! gadgets.
//!
//===----------------------------------------------------------------------===//

#include "scanner.hpp"

#include "core/error.hpp"
#include "utils/io.hpp"

#include <elf.h>
#include <expected>
#include <print>

namespace rop
{

core::Result<std::size_t> find_ret_instructions(
    std::span<const std::byte> buf,
    std::size_t ctx_bytes
)
{
    auto get_slice = [buf](std::size_t offset, std::size_t size)
        -> std::expected<std::span<const std::byte>, core::Error>
    {
        if (offset + size > buf.size() || offset + size < offset)
        {
            return std::unexpected(core::Error("offset out of bounds"));
        }

        return buf.subspan(offset, size);
    };

    // 1. read and validate ELF header
    if (buf.size() < sizeof(Elf64_Ehdr))
    {
        return std::unexpected(core::Error("buffer too small for ELF header"));
    }

    const auto *ehdr = reinterpret_cast<const Elf64_Ehdr *>(buf.data());

    // verify magic bytes first
    if (std::memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0)
    {
        return std::unexpected(core::Error("not a valid ELF binary (missing ELFMAG signature)"));
    }

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
    {
        return std::unexpected(
            core::Error("unsupported ELF architecture class (only 64-bit ELF is supported)")
        );
    }

    // read section headers
    if (ehdr->e_shoff == 0 || ehdr->e_shnum == 0)
    {
        return std::unexpected(core::Error("binary does not contain section headers"));
    }

    if (ehdr->e_shstrndx >= ehdr->e_shnum)
    {
        return std::unexpected(
            core::Error("corrupted ELF: invalid section header string table index")
        );
    }

    // 2. locate section headers table
    const std::size_t sh_table_size = sizeof(Elf64_Shdr) * ehdr->e_shnum;
    auto sh_slice = get_slice(ehdr->e_shoff, sh_table_size);
    if (!sh_slice)
    {
        return std::unexpected(core::Error("corrupted ELF: section headers exceed buffer bounds"));
    }

    const auto *sh_table = reinterpret_cast<const Elf64_Shdr *>(sh_slice->data());

    // 3. locate section header string table (.shstrtab)
    const Elf64_Shdr &shstr = sh_table[ehdr->e_shstrndx];
    if (shstr.sh_size == 0)
    {
        return std::unexpected(
            core::Error("corrupted ELF: section header string table size is zero")
        );
    }

    auto shstr_slice = get_slice(shstr.sh_offset, shstr.sh_size);
    if (!shstr_slice)
    {
        return std::unexpected(
            core::Error("corrupted ELF: section header string table exceeds buffer bounds")
        );
    }

    const char *shstrtab = reinterpret_cast<const char *>(shstr_slice->data());

    // 4. locate .text section with bounds checking
    const Elf64_Shdr *text_sh = nullptr;
    for (std::size_t i = 0; i < ehdr->e_shnum; ++i)
    {
        if (sh_table[i].sh_name >= shstr.sh_size)
        {
            continue; // ignore corrupted string table indices
        }

        const char *name = shstrtab + sh_table[i].sh_name;
        if (std::strcmp(name, ".text") == 0)
        {
            text_sh = &sh_table[i];
            break;
        }
    }

    if (!text_sh)
    {
        return std::unexpected(core::Error("section '.text' not found in binary"));
    }

    // 5. read .text Section Contents
    const Elf64_Off text_offset = text_sh->sh_offset;
    const Elf64_Xword text_size = text_sh->sh_size;
    const Elf64_Addr text_addr = text_sh->sh_addr;

    if (text_size == 0)
    {
        return 0; // empty .text section
    }

    auto text_slice = get_slice(text_offset, text_size);
    if (!text_slice)
    {
        return std::unexpected(core::Error(".text section offset exceeds binary bounds"));
    }

    print_section_info(text_offset, text_size, text_addr);

    // 6. scan for RET instructions
    std::size_t ret_count = 0;
    for (std::size_t i = 0; i < text_size; ++i)
    {
        if ((*text_slice)[i] == RET_OPCODE)
        {
            ++ret_count;
            std::size_t ctx_start = (i >= ctx_bytes ? i - ctx_bytes : 0);
            std::size_t ctx_end = i + 1; // include RET opcode

            std::println(
                "GADGET (ret) at file_offset=0x{:x} vaddr=0x{:x}",
                text_offset + i,
                text_addr + i
            );
            std::println("context ({} bytes before):", i - ctx_start);
            utils::io::print_bytes_hex(text_slice->data(), ctx_start, ctx_end);
            std::println("\n");
        }
    }

    return ret_count;
}

} // namespace rop
