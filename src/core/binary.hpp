//===----------------------------------------------------------------------===//
//
// Part of the ROPsmith Project.
// See LICENSE for license information.
//
//===----------------------------------------------------------------------===//
//!
//! \file core/binary.hpp
//! Definitions and utilities for handling binary files. Creates a class that
//! serves as an interface for different binary formats.
//!
//===----------------------------------------------------------------------===//

#ifndef CORE_BINARY_HPP
#define CORE_BINARY_HPP

#include <cstddef>
#include <cstring>
#include <expected>
#include <filesystem>
#include <vector>

//! \brief The file namespace contains classes and functions related to binary file handling.
namespace file
{

//! \brief Enumeration representing different binary file types.
enum class BinType
{
    UNK,
    ELF,
    PE,
    MACHO
};

//! \brief Enumeration representing different bitness types.
enum class Bitness
{
    UNK,
    x32,
    x64
};

//! \brief Enumeration representing different endianness types.
enum class Endian
{
    UNK,
    LITTLE,
    BIG
};

//! \brief Enumeration representing different CPU architectures.
enum class Arch
{
    UNK,
    x86,
    AMD64,
    ARM,
    AARCH64,
    RISCV,
    MIPS
};

std::string bin_type_to_string(const BinType &type);
std::string bitness_to_string(const Bitness &bitness);
std::string endian_to_string(const Endian &endian);
std::string arch_to_string(const Arch &arch);

//! \brief A class representing a binary file.
class Binary
{
  public:
    Binary();

    //! \brief Create a binary file object.
    //!
    //! \param path The path to the binary file.
    //! \throw std::runtime_error if the file info cannot be loaded.
    explicit Binary(const std::filesystem::path &path);

    //! \brief Load a binary file from the specified path.
    //!
    //! \param path The path to the binary file.
    //! \return void if successful, a core::Error otherwise.
    [[nodiscard]] std::expected<void, std::string> load(const std::filesystem::path &path) noexcept;

    //! \brief Save the binary file to the specified path.
    //!
    //! \param path The path to the binary file.
    //! \return void if successful, a core::Error otherwise.
    [[nodiscard]] std::expected<void, std::string> save(
        const std::filesystem::path &path
    ) const noexcept;

    //! \brief Get the path to the binary file.
    //!
    //! \return The path to the binary file.
    [[nodiscard]] const std::filesystem::path &path() const noexcept
    {
        return this->path_;
    }

    //! \brief Get the raw binary data.
    //!
    //! \return A pointer to the raw binary data.
    [[nodiscard]] const std::byte *data() const noexcept
    {
        return this->data_.data();
    }

    //! \brief Get the size of the binary data.
    //!
    //! \return The size of the binary data in bytes.
    [[nodiscard]] std::size_t size() const noexcept
    {
        return this->data_.size();
    }

    //! \brief Get the binary data as a vector of bytes.
    //!
    //! \return A reference to the vector containing the binary data.
    [[nodiscard]] const std::vector<std::byte> &vector() const noexcept
    {
        return this->data_;
    }

    //! \brief Get the type of the binary file.
    //!
    //! \return The type of the binary file.
    [[nodiscard]] const BinType &type() const noexcept
    {
        return this->type_;
    }

    //! \brief Get the bitness of the binary file.
    //!
    //! \return The bitness of the binary file.
    [[nodiscard]] const Bitness &bitness() const noexcept
    {
        return this->bitness_;
    }

    //! \brief Get the endianness of the binary file.
    //!
    //! \return The endianness of the binary file.
    [[nodiscard]] const Endian &endian() const noexcept
    {
        return this->endian_;
    }

    //! \brief Get the architecture of the binary file.
    //!
    //! \return The architecture of the binary file.
    [[nodiscard]] const Arch &arch() const noexcept
    {
        return this->arch_;
    }

    //! \brief Access a byte at the specified index.
    //!
    //! \param idx The index of the byte to access.
    //! \return The byte at the specified index on success, std::unexpected otherwise.
    [[nodiscard]] std::expected<std::byte, std::string> operator[](std::size_t idx) const noexcept;

  private:
    std::filesystem::path path_;
    std::vector<std::byte> data_;
    BinType type_;
    Bitness bitness_;
    Endian endian_;
    Arch arch_;

    //! \brief Find the type of the binary file.
    void find_type();

    //! \brief Find the bitness of the binary file.
    void find_bitness();

    //! \brief Find the endianness of the binary file.
    void find_endian();

    //! \brief Find the architecture of the binary file.
    void find_arch();

    //! \brief Read bytes from the binary data into a variable.
    //!
    //! \tparam T The type of the variable to read into.
    //! \param buf The buffer containing the binary data.
    //! \param offset The offset in the buffer to start reading from.
    //! \param out The variable to read the data into.
    //! \return True if the read was successful, false otherwise.
    template <typename T> bool read_bytes(const std::vector<std::byte> &buf, size_t offset, T &out);
};

//! \brief Print information about the binary file.
//!
//! \param binary The binary file to print information about.
void print_binary_info(const Binary &binary);

template <typename T>
bool Binary::read_bytes(const std::vector<std::byte> &buf, size_t offset, T &out)
{
    if (offset + sizeof(T) > buf.size())
    {
        return false;
    }

    std::memcpy(&out, buf.data() + offset, sizeof(T));
    return true;
}

} // namespace file

#endif // CORE_BINARY_HPP
