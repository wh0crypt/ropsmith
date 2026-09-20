//===----------------------------------------------------------------------===//
//
// Part of the ROPsmith Project.
// See LICENSE for license information.
//
//===----------------------------------------------------------------------===//
//!
//! \file utils/io.cpp
//! Implementation of utility functions for file I/O, printing, and general
//! helpers. These functions are used internally by ROPsmith scanning routines.
//!
//===----------------------------------------------------------------------===//

#include "io.hpp"

#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <system_error>
#include <vector>
#include <zlib.h>

namespace utils::io
{

void FileCloser::operator()(std::FILE *fp) const noexcept
{
    if (fp)
    {
        std::fclose(fp);
    }
}

core::Result<std::vector<std::byte>> read_file_to_buffer(const std::filesystem::path &path) noexcept
{
    std::vector<std::byte> buf;

    std::error_code ec;
    const std::uintmax_t size = std::filesystem::file_size(path, ec);
    if (ec)
    {
        return std::unexpected(
            core::Error(ec, std::format("cannot determine size of '{}'", path.string()))
        );
    }

    if (size == 0)
    {
        return std::unexpected(core::Error(std::format("file '{}' is empty", path.string())));
    }

    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        return std::unexpected(
            core::Error(std::format("could not open file '{}' for reading", path.string()))
        );
    }

    try
    {
        buf.resize(size);
    }
    catch (const std::bad_alloc &)
    {
        return std::unexpected(
            core::Error(
                std::format("out of memory allocating {} bytes for '{}'", size, path.string())
            )
        );
    }

    file.read(reinterpret_cast<char *>(buf.data()), static_cast<std::streamsize>(size));

    if (!file || file.gcount() != static_cast<std::streamsize>(size))
    {
        return std::unexpected(core::Error(std::format("incomplete read of '{}'", path.string())));
    }

    return buf;
}

core::Result<void> write_buffer_to_file(
    const std::vector<std::byte> &buf,
    const std::filesystem::path &path,
    unsigned long expected_crc
) noexcept
{
    std::ofstream file(path, std::ios::binary);
    if (!file)
    {
        return std::unexpected(
            core::Error(std::format("could not open file '{}' for writing", path.string()))
        );
    }

    file.write(
        reinterpret_cast<const char *>(buf.data()),
        static_cast<std::streamsize>(buf.size())
    );

    file.flush();
    if (!file)
    {
        return std::unexpected(
            core::Error(std::format("failed to write buffer data to '{}'", path.string()))
        );
    }
    file.close();

    // verify written file size on disk
    std::error_code ec;
    const std::uintmax_t written_size = std::filesystem::file_size(path, ec);
    if (ec)
    {
        return std::unexpected(
            core::Error(ec, std::format("failed to inspect written file '{}'", path.string()))
        );
    }

    if (written_size != buf.size())
    {
        return std::unexpected(
            core::Error(
                std::format(
                    "size mismatch after write of '{}' (expected {} bytes, found {})",
                    path.string(),
                    buf.size(),
                    written_size
                )
            )
        );
    }

    // optional CRC verification
    if (expected_crc != 0)
    {
        auto actual_crc = compute_crc32(buf);
        if (!actual_crc)
        {
            return std::unexpected(actual_crc.error());
        }

        if (*actual_crc != expected_crc)
        {
            return std::unexpected(
                core::Error(
                    std::format(
                        "CRC32 mismatch for '{}' (expected 0x{:x}, got 0x{:x})",
                        path.string(),
                        expected_crc,
                        *actual_crc
                    )
                )
            );
        }
    }

    return {};
}

core::Result<unsigned long> compute_crc32(const std::vector<std::byte> &buf) noexcept
{
    if (buf.empty())
    {
        return 0UL;
    }

    // process in chunks to prevent integer overflow on buffers > 4 GB
    unsigned long current_crc = crc32(0L, Z_NULL, 0);
    const std::byte *ptr = buf.data();
    std::size_t remaining = buf.size();

    while (remaining > 0)
    {
        const unsigned int chunk = static_cast<unsigned int>(
            std::min<std::size_t>(remaining, static_cast<std::size_t>(UINT_MAX))
        );
        current_crc = crc32(current_crc, reinterpret_cast<const Bytef *>(ptr), chunk);
        ptr += chunk;
        remaining -= chunk;
    }

    return current_crc;
}

core::Result<std::FILE *> open_binary(const std::filesystem::path &path) noexcept
{
#if defined(_WIN32)
    std::FILE *file = _wfopen(path.c_str(), L"rb");
#else
    std::FILE *file = std::fopen(path.c_str(), "rb");
#endif

    if (!file)
    {
        std::error_code ec(errno, std::generic_category());
        return std::unexpected(
            core::Error(ec, std::format("failed to open binary file '{}'", path.string()))
        );
    }

    return file;
}

} // namespace utils::io
