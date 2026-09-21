//===----------------------------------------------------------------------===//
//
// Part of the ROPsmith Project.
// See LICENSE for license information.
//
//===----------------------------------------------------------------------===//
///
/// \file tests/test_binary.cpp
/// Unit tests for the `Binary` module and class.
///
//===----------------------------------------------------------------------===//

#include <unistd.h>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

#include "core/binary.hpp"

#include <array>
#include <atomic>
#include <criterion/criterion.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unistd.h>

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

namespace
{

// plain text non-ELF fixture
constexpr std::string_view NON_ELF_CONTENT = "This is definitely not an ELF binary file.";

// minimal valid 64-bit Little-Endian x86_64 ELF Header fixture
// clang-format off
constexpr std::array<uint8_t, 64> MINIMAL_ELF64_HEADER = {
    0x7f, 'E', 'L', 'F',       // e_ident[EI_MAG0..3]
    ELFCLASS64,                // e_ident[EI_CLASS]
    ELFDATA2LSB,               // e_ident[EI_DATA]
    EV_CURRENT,                // e_ident[EI_VERSION]
    ELFOSABI_SYSV,             // e_ident[EI_OSABI]
    0, 0, 0, 0, 0, 0, 0, 0,    // e_ident pad
    0x02, 0x00,                // e_type = ET_EXEC
    0x3e, 0x00,                // e_machine = EM_X86_64 (62)
    0x01, 0x00, 0x00, 0x00,    // e_version = EV_CURRENT
    0x00, 0x00, 0x40, 0x00,    // e_entry = 0x400000
    0x00, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00,    // e_phoff = 64
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,    // e_shoff = 0
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,    // e_flags = 0
    0x40, 0x00,                // e_ehsize = 64
    0x38, 0x00,                // e_phentsize = 56
    0x01, 0x00,                // e_phnum = 1
    0x40, 0x00,                // e_shentsize = 64
    0x00, 0x00,                // e_shnum = 0
    0x00, 0x00                 // e_shstrndx = 0
};
// clang-format on

std::atomic<uint64_t> g_fixture_counter{0};

// RAII helper to create and automatically wipe temporary test fixtures
class TempFixture
{
  public:
    explicit TempFixture(const std::string_view prefix, const uint8_t *data, std::size_t size)
    {
        const auto unique_id = g_fixture_counter.fetch_add(1, std::memory_order_relaxed);
        const auto filename = std::format("ropsmith_{}_{}_{}.bin", ::getpid(), unique_id, prefix);
        this->path_ = std::filesystem::temp_directory_path() / filename;

        std::ofstream out(path_, std::ios::binary | std::ios::trunc);
        if (size > 0 && data != nullptr)
        {
            out.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(size));
        }
        out.flush();
    }

    ~TempFixture()
    {
        std::error_code ec;
        std::filesystem::remove(path_, ec);
    }

    [[nodiscard]] const std::filesystem::path &path() const noexcept
    {
        return path_;
    }

  private:
    std::filesystem::path path_;
};

} // namespace

Test(TestBinary, creates_bin_from_empty_file)
{
    TempFixture fixture("empty", nullptr, 0);

    cr_expect_throw(
        file::Binary binary(fixture.path()),
        std::runtime_error,
        "Expected exception loading empty file: %s",
        fixture.path().string().c_str()
    );
}

Test(TestBinary, creates_empty_bin)
{
    file::Binary binary;

    cr_expect_eq(binary.size(), 0, "Expected size 0 for empty binary");
    cr_expect_eq(binary.type(), file::BinType::UNK, "Expected BinType::UNK for empty binary");
    cr_expect_eq(binary.bitness(), file::Bitness::UNK, "Expected Bitness::UNK for empty binary");
    cr_expect_eq(binary.endian(), file::Endian::UNK, "Expected Endian::UNK for empty binary");
    cr_expect_eq(binary.arch(), file::Arch::UNK, "Expected Arch::UNK for empty binary");
}

Test(TestBinary, creates_bin_from_nonexistent_file)
{
    const std::filesystem::path nonexistent =
        std::filesystem::temp_directory_path() / std::format("nonexistent_{}.bin", ::getpid());

    cr_expect_throw(
        file::Binary binary(nonexistent),
        std::runtime_error,
        "Expected exception loading non-existent path: %s",
        nonexistent.string().c_str()
    );
}

Test(TestBinary, load_invalid_file)
{
    const std::filesystem::path nonexistent =
        std::filesystem::temp_directory_path() / std::format("nonexistent_{}.bin", ::getpid());
    file::Binary binary;

    auto res = binary.load(nonexistent);
    cr_expect(
        !res.has_value(),
        "Expected failure for non-existent file '%s', but it succeeded",
        nonexistent.string().c_str()
    );
}

Test(TestBinary, save_without_load)
{
    file::Binary binary;
    const std::filesystem::path temp_save =
        std::filesystem::temp_directory_path() / std::format("save_empty_{}.bin", ::getpid());

    auto res = binary.save(temp_save);
    cr_expect(!res.has_value(), "Expected save() to return error on unloaded binary");
}

Test(TestBinary, save_to_invalid_path)
{
    TempFixture fixture("valid", MINIMAL_ELF64_HEADER.data(), MINIMAL_ELF64_HEADER.size());
    file::Binary binary(fixture.path());

    const std::filesystem::path invalid_path = "/proc/invalid_non_writable_path/ropsmith.bin";
    auto res = binary.save(invalid_path);
    cr_expect(
        !res.has_value(),
        "Expected error saving to invalid path '%s'",
        invalid_path.string().c_str()
    );
}

Test(TestBinary, save_and_load_binary)
{
    TempFixture fixture("valid", MINIMAL_ELF64_HEADER.data(), MINIMAL_ELF64_HEADER.size());
    file::Binary binary(fixture.path());

    const std::filesystem::path temp_path =
        std::filesystem::temp_directory_path() / std::format("roundtrip_{}.bin", ::getpid());

    auto save_res = binary.save(temp_path);
    cr_expect(
        save_res.has_value(),
        "Save failed: %s",
        save_res ? "" : save_res.error().message().c_str()
    );

    file::Binary loaded_bin;
    auto load_res = loaded_bin.load(temp_path);
    cr_expect(
        load_res.has_value(),
        "Load failed: %s",
        load_res ? "" : load_res.error().message().c_str()
    );

    cr_expect_eq(loaded_bin.size(), binary.size(), "Size mismatch after save/load");
    cr_expect_eq(loaded_bin.type(), binary.type(), "Type mismatch after save/load");
    cr_expect_eq(loaded_bin.bitness(), binary.bitness(), "Bitness mismatch after save/load");
    cr_expect_eq(loaded_bin.endian(), binary.endian(), "Endianness mismatch after save/load");
    cr_expect_eq(loaded_bin.arch(), binary.arch(), "Architecture mismatch after save/load");

    std::error_code ec;
    std::filesystem::remove(temp_path, ec);
}

Test(TestBinary, access_byte_at_index)
{
    TempFixture fixture("valid", MINIMAL_ELF64_HEADER.data(), MINIMAL_ELF64_HEADER.size());
    file::Binary binary(fixture.path());

    auto b0 = binary[0];
    auto b1 = binary[1];
    auto b2 = binary[2];
    auto b3 = binary[3];

    cr_assert(b0.has_value() && b1.has_value() && b2.has_value() && b3.has_value());
    cr_expect_eq(*b0, std::byte{0x7f}, "Expected first byte to be 0x7f");
    cr_expect_eq(*b1, std::byte{'E'}, "Expected second byte to be 'E'");
    cr_expect_eq(*b2, std::byte{'L'}, "Expected third byte to be 'L'");
    cr_expect_eq(*b3, std::byte{'F'}, "Expected fourth byte to be 'F'");

    auto out_of_bounds = binary[binary.size()];
    cr_expect(
        !out_of_bounds.has_value(),
        "Expected error accessing out of bounds index %zu",
        binary.size()
    );
}

Test(TestBinary, type_bitness_endian_arch)
{
    TempFixture fixture("valid", MINIMAL_ELF64_HEADER.data(), MINIMAL_ELF64_HEADER.size());
    file::Binary binary(fixture.path());

    cr_expect_eq(binary.size(), MINIMAL_ELF64_HEADER.size(), "Size mismatch for valid ELF");
    cr_expect_eq(binary.type(), file::BinType::ELF, "Expected BinType::ELF");
    cr_expect_eq(binary.bitness(), file::Bitness::x64, "Expected Bitness::x64");
    cr_expect_eq(binary.endian(), file::Endian::LITTLE, "Expected Endian::LITTLE");
    cr_expect_eq(binary.arch(), file::Arch::AMD64, "Expected Arch::AMD64");
}

Test(TestBinary, multiple_loads)
{
    TempFixture fixture_elf("valid_elf", MINIMAL_ELF64_HEADER.data(), MINIMAL_ELF64_HEADER.size());
    TempFixture fixture_non_elf(
        "non_elf",
        reinterpret_cast<const uint8_t *>(NON_ELF_CONTENT.data()),
        NON_ELF_CONTENT.size()
    );

    file::Binary binary;
    auto load_result1 = binary.load(fixture_elf.path());
    cr_expect(
        load_result1.has_value(),
        "Expected binary load to succeed, but failed with: %s",
        load_result1 ? "" : load_result1.error().message().c_str()
    );
    cr_expect_eq(binary.type(), file::BinType::ELF, "Expected BinType::ELF after first load");

    auto load_result2 = binary.load(fixture_non_elf.path());
    cr_expect(
        load_result2.has_value(),
        "Expected binary load to succeed, but failed with: %s",
        load_result2 ? "" : load_result2.error().message().c_str()
    );
    cr_expect_eq(binary.type(), file::BinType::UNK, "Expected BinType::UNK after non-ELF load");
}

Test(TestBinary, verify_data_pointer)
{
    TempFixture fixture("valid", MINIMAL_ELF64_HEADER.data(), MINIMAL_ELF64_HEADER.size());
    file::Binary binary(fixture.path());

    const std::byte *data_ptr = binary.data();
    cr_expect_not_null(data_ptr, "Expected non-null data pointer");
    cr_expect_eq(data_ptr[0], std::byte{0x7f}, "Expected first byte to be 0x7f");
}

Test(TestBinary, verify_vector_contents)
{
    TempFixture fixture("valid", MINIMAL_ELF64_HEADER.data(), MINIMAL_ELF64_HEADER.size());
    file::Binary binary(fixture.path());

    const std::vector<std::byte> &vec = binary.vector();
    cr_expect_eq(vec.size(), binary.size(), "Expected vector size to match binary size");
    cr_expect_eq(vec[0], std::byte{0x7f}, "Expected first byte in vector to be 0x7f");
}

Test(TestBinary, path_accessor)
{
    TempFixture fixture("valid", MINIMAL_ELF64_HEADER.data(), MINIMAL_ELF64_HEADER.size());
    file::Binary binary(fixture.path());

    cr_expect_eq(
        binary.path(),
        fixture.path(),
        "Expected path accessor to return '%s', got '%s'",
        fixture.path().string().c_str(),
        binary.path().string().c_str()
    );
}

Test(TestBinary, null_data_on_empty_binary)
{
    file::Binary binary;
    cr_expect_null(binary.data(), "Expected null data pointer for empty binary");
}

Test(TestBinary, print_binary_info)
{
    TempFixture fixture("valid", MINIMAL_ELF64_HEADER.data(), MINIMAL_ELF64_HEADER.size());
    file::Binary binary(fixture.path());

    cr_expect_no_throw(
        print_binary_info(binary),
        std::runtime_error,
        "Expected print_binary_info to run without exceptions"
    );
}
