#pragma once
// DATABASE.CPP search block: TOC slots and record offsets as the routines read
// them. Meanings marked "inferred" come from how the routines use the values.
#include "database_detail.hpp"
#include "guest_values.hpp"
#include <cstdint>

namespace cm::aot::database::search {
using guest::Cpu;
constexpr std::uint32_t qsort = 0x10768300;  // generated: qsort4x.c

// PowerPC integer helpers shared by the routines of this block.
// extsb / extsh as a register value.
inline std::uint32_t extend_byte(std::uint32_t v) { return std::uint32_t(std::int32_t(std::int8_t(v))); }
inline std::uint32_t extend_half(std::uint32_t v) { return std::uint32_t(std::int32_t(std::int16_t(v))); }
// mulhw: the high word of the signed product.
inline std::uint32_t mulhw(std::uint32_t a, std::uint32_t b) {
    return std::uint32_t((std::int64_t(std::int32_t(a)) * std::int32_t(b)) >> 32);
}
// srawi: arithmetic shift right; XER[CA] is set when a negative value loses a 1 bit.
inline std::uint32_t shift_right_algebraic(Cpu& c, std::uint32_t value, unsigned n) {
    c.s.wr<1>(guest::reg::xer_ca, (value >> 31) && (value & ((1u << n) - 1)) != 0);
    return std::uint32_t(std::int32_t(value) >> n);
}

// TOC slots: each word holds the address of a game global.
namespace slot {
constexpr std::int32_t club_table = -0x7be8;       // pointer to the club table (records of 0x248 bytes)
constexpr std::int32_t club_count = -0x7b50;       // int32: clubs in the table
constexpr std::int32_t club_id_array = -0x765c;    // int32 array of club ids (first word of each club record)
constexpr std::int32_t club_id_compare = -0x7660;  // qsort comparator of the id array
constexpr std::int32_t ranked_array = -0x7a34;     // pointer to 0x1c-byte records (inferred: ranked entries)
constexpr std::int32_t ranked_count = -0x7a38;     // int32 count of those records
constexpr std::int32_t ranked_compare = -0x7a3c;   // qsort comparator of the 0x1c-byte records
constexpr std::int32_t ranked_floor = -0x751c;     // int32 (inferred: first record that counts as listed)
constexpr std::int32_t ranked_tail = -0x7518;      // pointer: receives the address of the entry floor records from the end
constexpr std::int32_t byte_table = -0x56b0;       // 14 bytes, the order 1..14 before sorting
constexpr std::int32_t byte_compare = -0x56b4;     // qsort comparator of the byte table
constexpr std::int32_t selector_byte = -0x39ee;    // a byte stored directly at r2 + offset
}
constexpr std::uint32_t resize = 0x107751b0;       // generated: (pointer, size) -> pointer
}
