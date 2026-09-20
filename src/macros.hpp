//===----------------------------------------------------------------------===//
//
// Part of the ROPsmith Project.
// See LICENSE for license information.
//
//===----------------------------------------------------------------------===//
//!
//! \file macros.hpp
//! Macros and constants for ROP gadget detection.
//!
//===----------------------------------------------------------------------===//

#ifndef MACROS_HPP
#define MACROS_HPP

#include <cstddef>

// default values
constexpr std::size_t DEFAULT_CONTEXT_BYTES = 16;
constexpr std::size_t DEFAULT_BYTES_PER_LINE = 16;

// instruction opcodes
constexpr std::byte RET_OPCODE = std::byte{0xC3};

#endif // MACROS_HPP
