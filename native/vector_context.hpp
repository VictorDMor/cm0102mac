#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
// Big-endian AltiVec storage used by the original exception-context code.
// This is guest register state; no host SIMD feature or pointer is required.
inline constexpr std::size_t vector_status_offset = 0x4400;
Vector128 load_vector_word(State& state, std::uint32_t base, std::uint32_t index);
std::uint32_t vector_word_for_address(const Vector128& value, std::uint32_t address);
Vector128 vector_status_register(const State& state);
void move_to_vector_status(State& state, const Vector128& value);
}
