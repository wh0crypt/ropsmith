//===----------------------------------------------------------------------===//
//
// Part of the ROPsmith Project.
// See LICENSE for license information.
//
//===----------------------------------------------------------------------===//
//!
//! \file core/project.cpp
//! Functions for scanning ELF binaries and detecting ROP gadgets.
//!
//===----------------------------------------------------------------------===//

#include "project.hpp"

#include "rop/scanner.hpp"

#include <exception>
#include <format>
#include <print>

namespace project
{

void print_info()
{
    std::println(
        "=== ROPsmith ===\n"
        "Version: {}\n"
        "Description: ROP gadget finder & chain generator.\n"
        "Build: {} {}\n"
        "================\n",
        VERSION,
        __DATE__,
        __TIME__
    );
}

core::Result<std::size_t> scan_bin(const file::Binary &bin, std::size_t ctx_bytes) noexcept
{
    try
    {
        return rop::find_ret_instructions(bin.vector(), ctx_bytes);
    }
    catch (const std::exception &e)
    {
        return std::unexpected(core::Error(std::format("scan_bin error: {}", e.what())));
    }
}

} // namespace project
