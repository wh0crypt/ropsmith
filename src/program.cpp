//===----------------------------------------------------------------------===//
//
// Part of the ROPsmith Project.
// See LICENSE for license information.
//
//===----------------------------------------------------------------------===//
//!
//! \file program.cpp
//! Implementation of functions for command-line argument handling.
//!
//===----------------------------------------------------------------------===//

#include "program.hpp"

#include "core/binary.hpp"
#include "core/error.hpp"
#include "core/project.hpp"

#include <charconv>
#include <exception>
#include <filesystem>
#include <format>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <system_error>

namespace program
{

core::Result<ProgramOptions> parse_arguments(std::span<char *> args) noexcept
{
    if (args.empty())
    {
        return std::unexpected(core::Error("argument list cannot be empty"));
    }

    ProgramOptions options;

    for (auto it = args.begin() + 1; it != args.end(); ++it)
    {
        if (*it == nullptr)
        {
            continue;
        }

        const std::string_view arg = *it;
        if (arg == "--help" || arg == "-h")
        {
            options.show_help = true;
            return options;
        }

        if (arg == "-v" || arg == "--verbose")
        {
            options.verbose = true;
            continue;
        }

        if (arg == "--context" || arg == "-c")
        {
            if (++it == args.end() || *it == nullptr)
            {
                return std::unexpected(core::Error("flag '--context' requires a numeric value"));
            }

            const std::string_view val = *it;
            int parsed_value = 0;
            const auto [ptr, ec] =
                std::from_chars(val.data(), val.data() + val.size(), parsed_value);

            if (ec != std::errc{} || ptr != val.data() + val.size() || parsed_value < 0)
            {
                return std::unexpected(
                    core::Error(std::format("invalid context byte count '{}'", val))
                );
            }

            options.ctx_bytes = static_cast<std::size_t>(parsed_value);
            continue;
        }

        if (arg == "--file" || arg == "-f")
        {
            if (++it == args.end() || *it == nullptr)
            {
                return std::unexpected(core::Error("flag '--file' requires a file path"));
            }

            const std::filesystem::path path = *it;
            std::error_code ec;

            if (!std::filesystem::exists(path, ec))
            {
                if (ec)
                {
                    return std::unexpected(
                        core::Error(ec, std::format("cannot access file '{}'", *it))
                    );
                }

                return std::unexpected(
                    core::Error(std::format("binary file '{}' does not exist", *it))
                );
            }

            if (!std::filesystem::is_regular_file(path, ec))
            {
                return std::unexpected(core::Error(std::format("'{}' is not a regular file", *it)));
            }

            options.binary_path = path;
            continue;
        }

        return std::unexpected(core::Error(std::format("unknown argument: '{}'", *it)));
    }

    if (!options.show_help && options.binary_path.empty())
    {
        return std::unexpected(core::Error("missing required argument: -f / --file <binary>"));
    }

    return options;
}

void print_usage()
{
    std::println("Usage: ropsmith [--help] [--verbose] [--context N] --file <binary>");
}

void print_help()
{
    print_usage();
    std::println(
        "\nOptions:\n"
        "-h, --help\t\tShow this help message and exit.\n"
        "-v, --verbose\t\tEnable verbose diagnostics and detailed error traces.\n"
        "-f, --file <binary>\tSelect a binary file to analyze.\n"
        "-c N, --context N\tSet the number of context bytes (default: {}).\n"
        "Example Usage:\n"
        "\tropsmith -f /bin/ls -c 16 -v",
        DEFAULT_CONTEXT_BYTES
    );
}

core::Result<void> handle(const ProgramOptions &opts) noexcept
{
    std::println(
        "scanning target {} (context={} bytes):\n",
        opts.binary_path.string(),
        opts.ctx_bytes
    );

    try
    {
        file::Binary binary(opts.binary_path);
        file::print_binary_info(binary);

        auto res = project::scan_bin(binary, opts.ctx_bytes);
        if (!res)
        {
            return std::unexpected(res.error());
        }

        std::println("found {} RET instructions.", *res);
    }
    catch (const std::exception &e)
    {
        return std::unexpected(
            core::Error(std::format("exception raised during binary processing: {}", e.what()))
        );
    }

    return {};
}

} // namespace program
