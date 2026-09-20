//===----------------------------------------------------------------------===//
//
// Part of the ROPsmith Project.
// See LICENSE for license information.
//
//===----------------------------------------------------------------------===//
//!
//! \file core/ropsmith.hpp
//! Main public header for ROPsmith. Provides high-level functions and
//! utility functions for printing info and handling user interaction.
//!
//===----------------------------------------------------------------------===//

#ifndef CORE_PROJECT_HPP
#define CORE_PROJECT_HPP

#include "core/binary.hpp"
#include "error.hpp"
#include "macros.hpp"

#include <cstddef>
#include <cstdio>
#include <string_view>

namespace project
{

//! Project-wide version constant
constexpr std::string_view VERSION = "0.1";

//! \brief Prints basic information about the tool.
//!
//! This function prints the banner, version, or other relevant info to
//! the standard output. Can be called at the start of main().
void print_info();

//! \brief Scans the specified binary for ROP gadgets.
//!
//! This is a high-level wrapper around the internal scanning functions.
//! \param bin The binary file to scan.
//! \param ctx_bytes Number of context bytes to include around each gadget.
//! \return Returns the number of 'ret' instructions found if successful, core::Error
//! otherwise.
[[nodiscard]] core::Result<std::size_t> scan_bin(
    const file::Binary &bin,
    std::size_t ctx_bytes = DEFAULT_CONTEXT_BYTES
) noexcept;

} // namespace project

#endif // CORE_PROJECT_HPP
