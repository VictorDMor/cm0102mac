#pragma once
#include "aot_runtime.hpp"

namespace cm::aot::compiler_arithmetic {
// Full named arithmetic operations. The adapter owns instruction charges,
// bounded-budget selection and process-local runtime diagnostics.
void unsigned_quotient(State&);
void signed_quotient(State&);
void unsigned_remainder(State&);
void signed_remainder(State&);
void double_to_unsigned_word(State&);
void signed_pair_to_double(State&);
void signed_pair_to_single(State&);
void double_to_signed_pair(State&);
// Exact original instruction charge, including the software zero-divisor path.
std::uint64_t unsigned_quotient_instruction_count(const State&);
}
