//===----------------------------------------------------------------------===//
//
// Part of the ROPsmith Project.
// See LICENSE for license information.
//
//===----------------------------------------------------------------------===//
//!
//! \file core/rop/scanner.hpp
//! Functions for scanning ELF binaries and detecting ROP gadgets.
//!
//===----------------------------------------------------------------------===//

#ifndef CORE_ROP_SCANNER_HPP
#define CORE_ROP_SCANNER_HPP

#include "core/error.hpp"
#include "macros.hpp"

#include <cstddef>
#include <cstring>
#include <print>
#include <span>

#if defined(__has_include)
#if __has_include(<elf.h>)
#include <elf.h>
#elif __has_include("include/elf.h")
#include "include/elf.h"
#elif __has_include("elf.h")
#include "elf.h"
#else
#error "Could not locate elf.h on this platform"
#endif
#else
#include <elf.h>
#endif // ELF headers

namespace rop
{

//! \brief Prints information about a section.
//!
//! \param offset Offset of the section in the file.
//! \param size Size of the section.
//! \param addr Virtual address of the section.
inline void print_section_info(std::uint64_t offset, std::uint64_t size, std::uint64_t addr)
{
    std::println(".text offset=0x{:x} size=0x{:x} vaddr=0x{:x}\n", offset, size, addr);
}

//! \brief Scans the .text section of an ELF binary for 'ret' instructions.
//!
//! This function parses the file header to locate the .text section, and counts occurrences of the
//! 'ret' opcode. The \p context parameter specifies how many bytes of surrounding context to
//! consider for each gadget.
//!
//! \param buf Buffer containing the ELF binary file data.
//! \param ctx_size Number of surrounding bytes to include per gadget.
//! \return Returns the number of 'ret' instructions found if successful, core::Error
//! otherwise.
[[nodiscard]] core::Result<std::size_t> find_ret_instructions(
    std::span<const std::byte> buf,
    std::size_t ctx_bytes = DEFAULT_CONTEXT_BYTES
);

} // namespace rop

#endif // CORE_ROP_SCANNER_HPP
