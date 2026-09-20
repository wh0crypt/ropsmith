//===----------------------------------------------------------------------===//
//
// Part of the ROPsmith Project.
// See LICENSE for license information.
//
//===----------------------------------------------------------------------===//
//!
//! \file utils/io.hpp
//! Utility functions for file I/O, printing, and general helpers.
//! These functions are used internally by ROPsmith scanning routines.
//!
//===----------------------------------------------------------------------===//

#ifndef UTILS_IO_HPP
#define UTILS_IO_HPP

#include "core/error.hpp"
#include "macros.hpp"

#include <filesystem>
#include <print>
#include <vector>

namespace utils::io
{

//! \brief Closes a FILE pointer.
//!
//! \param fp Pointer to the FILE to close.
struct FileCloser
{
    void operator()(std::FILE *fp) const noexcept;
};

//! \brief Reads the entire contents of a file into a buffer.
//!
//! \param path Path to the file to read.
//! \return The buffer containing the data if successful, core::Error otherwise.
[[nodiscard]] core::Result<std::vector<std::byte>> read_file_to_buffer(
    const std::filesystem::path &path
) noexcept;

//! \brief Writes the contents of a buffer to a file.
//!
//! \param buf Reference to the buffer to write.
//! \param path Path to the file to write to.
//! \param expected_crc Optional expected CRC32 checksum for verification.
//! \return void if successful, core::Error otherwise.
[[nodiscard]] core::Result<void> write_buffer_to_file(
    const std::vector<std::byte> &buf,
    const std::filesystem::path &path,
    unsigned long expected_crc = 0
) noexcept;

//! \brief Computes the CRC32 checksum of the given data.
//!
//! \param buf Reference to the buffer containing the data.
//! \return The CRC32 checksum if successful, core::Error otherwise.
[[nodiscard]] core::Result<unsigned long> compute_crc32(const std::vector<std::byte> &buf) noexcept;

//! \brief Prints bytes in hexadecimal format.
//!
//! \tparam T Type of the buffer (e.g., std::vector<std::byte>).
//! \param buf Reference to the buffer containing bytes to print.
//! \param start Starting index in the buffer.
//! \param end Ending index in the buffer.
template <typename T> void print_bytes_hex(const T &buf, std::size_t start, std::size_t end)
{
    for (std::size_t i = start; i < end; ++i)
    {
        std::print("{} ", static_cast<unsigned int>(buf[i]));
        if ((i + 1) % DEFAULT_BYTES_PER_LINE == 0)
        {
            std::println("");
        }
    }
}

//! \brief Opens a binary file and returns a file pointer.
//!
//! \param path Path to the binary file.
//! \return Pointer to the opened file if successful, core::Error otherwise.
[[nodiscard]] core::Result<std::FILE *> open_binary(const std::filesystem::path &path) noexcept;

//! \brief Swaps the byte order of an integral value.
//!
//! \tparam T Integral type (e.g., uint16_t, uint32_t, uint64_t).
//! \param value The value to swap.
//! \return The value with swapped byte order.
template <typename T> constexpr T swap_bytes(const T &value) noexcept
{
    static_assert(std::is_integral_v<T>, "swap_bytes requires an integral type");
    static_assert(std::is_unsigned_v<T>, "swap_bytes requires an unsigned type");

#if __cpp_lib_byteswap >= 202110L
    return std::byteswap(value);
#else
    if constexpr (sizeof(T) == 1)
    {
        return value;
    }

    if constexpr (sizeof(T) == 2)
    {
        return static_cast<T>(((value & 0x00FFu) << 8) | ((value & 0xFF00u) >> 8));
    }

    if constexpr (sizeof(T) == 4)
    {
        return static_cast<T>(
            ((value & 0x000000FFu) << 24) | ((value & 0x0000FF00u) << 8) |
            ((value & 0x00FF0000u) >> 8) | ((value & 0xFF000000u) >> 24)
        );
    }

    if constexpr (sizeof(T) == 8)
    {
        return static_cast<T>(
            ((value & 0x00000000000000FFull) << 56) | ((value & 0x000000000000FF00ull) << 40) |
            ((value & 0x0000000000FF0000ull) << 24) | ((value & 0x00000000FF000000ull) << 8) |
            ((value & 0x000000FF00000000ull) >> 8) | ((value & 0x0000FF0000000000ull) >> 24) |
            ((value & 0x00FF000000000000ull) >> 40) | ((value & 0xFF00000000000000ull) >> 56)
        );
    }

    static_assert(sizeof(T) <= 8, "unsupported integer size");
#endif
}

} // namespace utils::io

#endif // UTILS_IO_HPP
