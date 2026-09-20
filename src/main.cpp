//===----------------------------------------------------------------------===//
//
// Part of the ROPsmith Project.
// See LICENSE for license information.
//
//===----------------------------------------------------------------------===//
//!
//! \file main.cpp
//! Entry point for the ROPsmith CLI tool.
//!
//===----------------------------------------------------------------------===//

#include "core/project.hpp"
#include "program.hpp"

#include <cstdio>
#include <cstdlib>
#include <print>
#include <span>

//! \brief Main entry point for the ROPsmith CLI tool.
//!
//! \param argc Argument count.
//! \param argv Argument vector.
//! \return Exit code.
int main(int argc, char **argv)
{
    const std::span<char *> args{argv, static_cast<std::size_t>(argc)};

    if (argc < 2)
    {
        program::print_usage();
        return EXIT_FAILURE;
    }

    auto opts = program::parse_arguments(args);
    if (!opts)
    {
        std::println(stderr, "ropsmith error: {}", opts.error());
        return EXIT_FAILURE;
    }

    if (opts->show_help)
    {
        program::print_help();
        return EXIT_SUCCESS;
    }

    project::print_info();

    auto exec = program::handle(*opts);
    if (!exec)
    {
        const auto &err = exec.error();

        if (opts->verbose)
        {
            std::println(stderr, "[DEBUG TRACE] {}", err.trace());
        }
        else
        {
            std::println(stderr, "ropsmith: error: {}", err);
        }

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
