#pragma once
// DATABASE.CPP's people block: shared helpers and record layouts, as far as the rewritten routines read
// them. Meanings marked "inferred" come from how the routines use the values.
#include "guest_abi.hpp"
#include "database_generation_detail.hpp"
#include "guest_values.hpp"

namespace cm::aot::database::people {
using guest::Cpu;
using guest::compare;
using guest::compare_logical;

// The generation block's loads (lwz / lbz into register `n`, answering the value) and sign extension.
using generation::extsb;
using generation::load32;
using generation::load8;
// mulhw: the high word of the signed 64-bit product (every reciprocal division of the compiler's inline draws).
inline std::uint32_t mulhw(std::uint32_t a, std::uint32_t b) {
    return std::uint32_t(std::uint64_t(std::int64_t(std::int32_t(a)) * std::int64_t(std::int32_t(b))) >> 32);
}

// The generated people table (TOC -0x7be0 holds the word that points at it): rows of 0x70 bytes, the
// number of rows in the word at TOC -0x7be4 (inferred: the people the generator creates).
namespace person_row {
constexpr std::int32_t table_slot = -0x7be0, count_slot = -0x7be4;
constexpr std::uint32_t stride = 0x70, nation = 0xc, first_name = 0x28, second_name = 0x2c, third_name = 0x30;
}
}
