//===----------------------------------------------------------------------===//
//
// Part of the ROPsmith Project.
// See LICENSE for license information.
//
//===----------------------------------------------------------------------===//
//!
//! \file program.hpp
//! \brief Functions for command-line argument handling.
//!
//===----------------------------------------------------------------------===//

#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#include "core/error.hpp"
#include "macros.hpp"

#include <filesystem>

namespace program
{

//! \brief Structure to hold parsed program options.
//!
//! This structure contains the binary path, context bytes, error messages,
//! error codes, and a flag to indicate if help should be shown.
struct ProgramOptions
{
    std::filesystem::path binary_path = "";
    std::size_t ctx_bytes = DEFAULT_CONTEXT_BYTES;
    bool show_help = false;
    bool verbose = false;
};

//! \brief Parse command-line arguments.
//!
//! This function processes the command-line arguments and populates
//! a `ProgramOptions` structure with the parsed values.
//!
//! \param args Non-owning view of the command-line argument vector.
//! \returns Struct with the program options on success, core::Error otherwise.
[[nodiscard]] core::Result<ProgramOptions> parse_arguments(std::span<char *> args) noexcept;

//! \brief Print usage information for the ROPsmith CLI tool.
void print_usage();

//! \brief Print detailed help information for the ROPsmith CLI tool.
void print_help();

//! \brief Main handler of the program.
//!
//! \param opts Parsed options from the CLI.
//! \returns void on success, core::Error otherwise.
[[nodiscard]] core::Result<void> handle(const ProgramOptions &opts) noexcept;

} // namespace program

#endif // PROGRAM_HPP
