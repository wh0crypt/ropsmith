//===----------------------------------------------------------------------===//
//
// Part of the ROPsmith Project.
// See LICENSE for license information.
//
//===----------------------------------------------------------------------===//
//!
//! \file core/binary.cpp
//! Implementations for handling binary files.
//!
//===----------------------------------------------------------------------===//

#include "binary.hpp"

#include "utils/io.hpp"

#include <exception>
#include <expected>
#include <format>
#include <print>
#include <stdexcept>

#if defined(__APPLE__) || defined(_WIN32)
#include "include/elf.h"
#else
#include <elf.h>
#endif // ELF headers

#if defined(_WIN32)
#include <windows.h>
#include <winnt.h>
#else
#include "include/winnt_min.h"
#endif // Windows headers

namespace file
{

std::string bin_type_to_string(const BinType &type)
{
    switch (type)
    {
        case BinType::ELF:
            return "ELF";
        case BinType::PE:
            return "PE";
        case BinType::MACHO:
            return "Mach-O";
        default:
            return "Unknown";
    }
}

std::string bitness_to_string(const Bitness &bitness)
{
    switch (bitness)
    {
        case Bitness::x32:
            return "32-bit";
        case Bitness::x64:
            return "64-bit";
        default:
            return "Unknown";
    }
}

std::string endian_to_string(const Endian &endian)
{
    switch (endian)
    {
        case Endian::LITTLE:
            return "LSB";
        case Endian::BIG:
            return "MSB";
        default:
            return "Unknown";
    }
}

std::string arch_to_string(const Arch &arch)
{
    switch (arch)
    {
        case Arch::x86:
            return "x86";
        case Arch::AMD64:
            return "x86_64";
        case Arch::ARM:
            return "arm";
        case Arch::AARCH64:
            return "aarch64";
        case Arch::RISCV:
            return "riscv";
        case Arch::MIPS:
            return "mips";
        default:
            return "Unknown";
    }
}

Binary::Binary()
{
    this->path_ = std::filesystem::path();
    this->data_.clear();
    this->type_ = BinType::UNK;
    this->bitness_ = Bitness::UNK;
    this->endian_ = Endian::UNK;
    this->arch_ = Arch::UNK;
}

Binary::Binary(const std::filesystem::path &path)
    : path_(path), type_(BinType::UNK), bitness_(Bitness::UNK), endian_(Endian::UNK),
      arch_(Arch::UNK)
{
    auto res = this->load(path);
    if (!res)
    {
        throw std::runtime_error(res.error());
    }
}

std::expected<void, std::string> Binary::load(const std::filesystem::path &path) noexcept
{
    if (std::filesystem::is_empty(path))
    {
        return std::unexpected(
            std::format("Binary::load() error: file '{}' is empty", path.string())
        );
    }

    auto success = utils::io::read_file_to_buffer(path);
    if (!success)
    {
        return std::unexpected(
            std::format("Binary::load() error:error loading binary file: {}", success.error())
        );
    }

    this->data_ = *success;

    this->find_type();
    this->find_bitness();
    this->find_endian();
    this->find_arch();

    return {};
}

std::expected<void, std::string> Binary::save(const std::filesystem::path &path) const noexcept
{
    if (this->data_.empty())
    {
        return std::unexpected("error saving binary file: no data to save");
    }

    auto success = utils::io::write_buffer_to_file(this->data_, path);
    if (!success)
    {
        return std::unexpected(std::format("error saving binary file: {}", success.error()));
    }

    return {};
}

void Binary::find_type()
{
    if (this->size() < 4)
    {
        this->type_ = BinType::UNK;
        return;
    }

    // ELF: 0x7F 'E' 'L' 'F'
    // ELF magic: 0x7F 'E' 'L' 'F'
    if (this->vector()[0] == std::byte{0x7F} && this->vector()[1] == std::byte{'E'} &&
        this->vector()[2] == std::byte{'L'} && this->vector()[3] == std::byte{'F'})
    {
        this->type_ = BinType::ELF;
        return;
    }

    // PE: bytes 0..1 = 'M' 'Z' (0x4D 0x5A)
    if (this->vector()[0] == std::byte{'M'} && this->vector()[1] == std::byte{'Z'})
    {
        this->type_ = BinType::PE;
        return;
    }

    // Mach-O: bytes 0..3 = magic
    // 0xFEEDFACE  -> 32-bit little endian
    // 0xCEFAEDFE  -> 32-bit big endian
    // 0xFEEDFACF  -> 64-bit little endian
    // 0xCFFAEDFE  -> 64-bit big endian
    std::uint32_t magic = 0;
    if (!read_bytes(this->vector(), 0, magic))
    {
        this->type_ = BinType::UNK;
        return;
    }

    if (magic == 0 || magic == 0xFFFFFFFF)
    {
        this->type_ = BinType::UNK;
        return;
    }

    switch (magic)
    {
        case 0xFEEDFACE:
        case 0xFEEDFACF:
        case 0xCEFAEDFE:
        case 0xCFFAEDFE:
            this->type_ = BinType::MACHO;
            return;
        default:
            this->type_ = BinType::UNK;
    }
}

void Binary::find_bitness()
{
    if (this->type() == BinType::ELF)
    {
        std::byte ei_class = this->vector()[4];

        if (this->size() > 5)
        {
            if (ei_class == std::byte{1})
            {
                this->bitness_ = Bitness::x32;
                return;
            }

            if (ei_class == std::byte{2})
            {
                this->bitness_ = Bitness::x64;
                return;
            }
        }

        this->bitness_ = Bitness::UNK;
        return;
    }

    if (this->type() == BinType::PE)
    {
        if (this->size() < 0x40)
        {
            this->bitness_ = Bitness::UNK;
            return;
        }

        // PE header offset is at 0x3C (4 bytes)
        std::uint32_t pe_offset = 0;
        if (!read_bytes(this->vector(), 0x3C, pe_offset))
        {
            this->bitness_ = Bitness::UNK;
            return;
        }

        // 4 bytes (offset) + 2 bytes (machine)
        if (pe_offset > this->size() - 6)
        {
            this->bitness_ = Bitness::UNK;
            return;
        }

        // Magic field is at offset pe_offset + 4 (2 bytes)
        uint16_t magic = 0;
        if (!read_bytes(this->vector(), pe_offset + 0x18, magic))
        {
            this->bitness_ = Bitness::UNK;
            return;
        }

        switch (magic)
        {
            case 0x10b:
                this->bitness_ = Bitness::x32;
                return;
            case 0x20b:
                this->bitness_ = Bitness::x64;
                return;
            default:
                this->bitness_ = Bitness::UNK;
                return;
        }
    }

    if (this->type() == BinType::MACHO)
    {
        uint32_t magic = 0;
        if (!read_bytes(this->vector(), 0, magic))
        {
            this->bitness_ = Bitness::UNK;
            return;
        }

        switch (magic)
        {
            case 0xFEEDFACE:
            case 0xCEFAEDFE:
                this->bitness_ = Bitness::x32;
                return;
            case 0xFEEDFACF:
            case 0xCFFAEDFE:
                this->bitness_ = Bitness::x64;
                return;
            default:
                this->bitness_ = Bitness::UNK;
                return;
        }
    }
}

void Binary::find_endian()
{
    if (this->size() < 8)
    {
        this->endian_ = Endian::UNK;
        return;
    }

    if (this->type() == BinType::ELF)
    {
        std::byte ei_data = this->vector()[5];

        if (ei_data == std::byte{1})
        {
            this->endian_ = Endian::LITTLE;
            return;
        }

        if (ei_data == std::byte{2})
        {
            this->endian_ = Endian::BIG;
            return;
        }

        this->endian_ = Endian::UNK;
        return;
    }

    if (this->type() == BinType::PE)
    {
        this->endian_ = Endian::LITTLE;
        return;
    }

    if (this->type() == BinType::MACHO)
    {
        // Mach-O: bytes 0..3 = magic
        // 0xFEEDFACE  -> 32-bit little endian
        // 0xCEFAEDFE  -> 32-bit big endian
        // 0xFEEDFACF  -> 64-bit little endian
        // 0xCFFAEDFE  -> 64-bit big endian
        std::uint32_t magic = 0;
        if (!read_bytes(this->vector(), 0, magic))
        {
            this->endian_ = Endian::UNK;
            return;
        }

        if (magic == 0 || magic == 0xFFFFFFFF)
        {
            this->endian_ = Endian::UNK;
            return;
        }

        switch (magic)
        {
            case 0xFEEDFACE:
            case 0xFEEDFACF:
                this->endian_ = Endian::LITTLE;
                return;
            case 0xCEFAEDFE:
            case 0xCFFAEDFE:
                this->endian_ = Endian::BIG;
                return;
            default:
                this->endian_ = Endian::UNK;
                return;
        }
    }

    this->endian_ = Endian::UNK;
}

void Binary::find_arch()
{
    if (this->size() < 4 || this->type() == BinType::UNK)
    {
        this->arch_ = Arch::UNK;
        return;
    }

    if (this->type() == BinType::ELF)
    {
        if (this->size() < 0x14)
        {
            this->arch_ = Arch::UNK;
            return;
        }

        // e_machine is at offset 0x12 (2 bytes) in ELF header
        uint16_t e_machine = 0;
        if (!read_bytes(this->vector(), 0x12, e_machine))
        {
            this->arch_ = Arch::UNK;
            return;
        }

        // Endianness handling
        if (this->endian() == Endian::BIG)
        {
            e_machine = utils::io::swap_bytes(e_machine);
        }

        switch (e_machine)
        {
            case EM_386:
                this->arch_ = Arch::x86;
                return;
            case EM_X86_64:
                this->arch_ = Arch::AMD64;
                return;
            case EM_ARM:
                this->arch_ = Arch::ARM;
                return;
            case EM_AARCH64:
                this->arch_ = Arch::AARCH64;
                return;
            case EM_RISCV:
                this->arch_ = Arch::RISCV;
                return;
            case EM_MIPS:
                this->arch_ = Arch::MIPS;
                return;
            default:
                this->arch_ = Arch::UNK;
                return;
        }
    }

    if (this->type() == BinType::PE)
    {
        if (this->size() < 0x40)
        {
            this->arch_ = Arch::UNK;
            return;
        }

        // PE header offset is at 0x3C (4 bytes)
        std::uint32_t pe_offset = 0;
        if (!read_bytes(this->vector(), 0x3C, pe_offset))
        {
            this->arch_ = Arch::UNK;
            return;
        }

        // 4 bytes (offset) + 2 bytes (machine)
        if (pe_offset > this->size() - 6)
        {
            this->arch_ = Arch::UNK;
            return;
        }

        // Machine field is at offset pe_offset + 4 (2 bytes)
        uint16_t machine = 0;
        if (!read_bytes(this->vector(), pe_offset + 4, machine))
        {
            this->arch_ = Arch::UNK;
            return;
        }

        switch (machine)
        {
            case IMAGE_FILE_MACHINE_UNKNOWN:
                this->arch_ = Arch::UNK;
                return;
            case IMAGE_FILE_MACHINE_I386:
                this->arch_ = Arch::x86;
                return;
            case IMAGE_FILE_MACHINE_AMD64:
                this->arch_ = Arch::AMD64;
                return;
            case IMAGE_FILE_MACHINE_ARM:
            case IMAGE_FILE_MACHINE_THUMB:
            case IMAGE_FILE_MACHINE_ARMNT:
                this->arch_ = Arch::ARM;
                return;
            case IMAGE_FILE_MACHINE_ARM64:
                this->arch_ = Arch::AARCH64;
                return;
            case IMAGE_FILE_MACHINE_MIPS16:
            case IMAGE_FILE_MACHINE_MIPSFPU:
            case IMAGE_FILE_MACHINE_MIPSFPU16:
                this->arch_ = Arch::MIPS;
                return;
            default:
                this->arch_ = Arch::UNK;
                return;
        }
    }

    if (this->type() == BinType::MACHO)
    {
        // magic (4) + cpu_type (4) + cpusubtype (4)
        if (this->size() < 12)
        {
            this->arch_ = Arch::UNK;
            return;
        }

        std::uint32_t magic = 0;
        if (!read_bytes(this->vector(), 0, magic))
        {
            this->arch_ = Arch::UNK;
            return;
        }

        // Fat Mach-O
        if (magic == 0xCAFEBABE || magic == 0xBEBAFECA || magic == 0xCAFED00D ||
            magic == 0xD00DFECA)
        {
            bool is_big_endian = (magic == 0xCAFEBABE || magic == 0xCAFED00D);
            std::uint32_t nfat_arch = 0;

            if (!read_bytes(this->vector(), 4, nfat_arch))
            {
                this->arch_ = Arch::UNK;
                return;
            }

            if (!is_big_endian)
            {
            }

            // fat_arch structure is 20 bytes
            std::size_t arch_offset = 8;
            for (std::uint32_t i = 0; i < nfat_arch; ++i)
            {
                if (arch_offset + 8 > this->size())
                {
                    break;
                }

                std::uint32_t cpu_type = 0;
                if (!read_bytes(this->vector(), arch_offset, cpu_type))
                {
                    break;
                }

                if (!is_big_endian)
                {
                    cpu_type = utils::io::swap_bytes(cpu_type);
                }

                switch (cpu_type)
                {
                    case 7: // CPU_TYPE_X86
                        this->arch_ = Arch::x86;
                        return;
                    case 0x01000007: // CPU_TYPE_X86_64
                        this->arch_ = Arch::AMD64;
                        return;
                    case 12: // CPU_TYPE_ARM
                        this->arch_ = Arch::ARM;
                        return;
                    case 0x0100000C: // CPU_TYPE_ARM64
                        this->arch_ = Arch::AARCH64;
                        return;
                    default:
                        break;
                }

                arch_offset += 20;
            }

            this->arch_ = Arch::UNK;
            return;
        }

        // Normal Mach-O
        std::uint32_t cpu_type = 0;
        if (!read_bytes(this->vector(), 4, cpu_type))
        {
            this->arch_ = Arch::UNK;
            return;
        }

        if (magic == 0 || magic == 0xFFFFFFFF)
        {
            this->arch_ = Arch::UNK;
            return;
        }

        if (this->endian() == Endian::BIG)
        {
            cpu_type = utils::io::swap_bytes(cpu_type);
        }

        switch (cpu_type)
        {
            case 7: // CPU_TYPE_X86
                this->arch_ = Arch::x86;
                return;
            case 0x01000007: // CPU_TYPE_X86_64
                this->arch_ = Arch::AMD64;
                return;
            case 12: // CPU_TYPE_ARM
                this->arch_ = Arch::ARM;
                return;
            case 0x0100000C: // CPU_TYPE_ARM64
                this->arch_ = Arch::AARCH64;
                return;
            default:
                this->arch_ = Arch::UNK;
                return;
        }
    }

    this->arch_ = Arch::UNK;
}

std::expected<std::byte, std::string> Binary::operator[](std::size_t idx) const noexcept
{
    try
    {
        return this->data_.at(idx);
    }
    catch (const std::exception &e)
    {
        return std::unexpected(std::format("Binary::operator[]: {}", e.what()));
    }
}

void print_binary_info(const Binary &binary)
{
    std::println(
        "binary info: {} {} {}, {}",
        bin_type_to_string(binary.type()),
        bitness_to_string(binary.bitness()),
        endian_to_string(binary.endian()),
        arch_to_string(binary.arch())
    );
}

} // namespace file
