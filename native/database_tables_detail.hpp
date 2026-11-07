#pragma once
// DATABASE.CPP's tables block: the TOC slots of the global tables, the record layouts the routines read and the
// instruction-level helpers shared by the files. Meanings marked "inferred" come from how the routines use them.
#include "database_generation_detail.hpp"
#include "guest_abi.hpp"
#include "guest_values.hpp"
#include <bit>

namespace cm::aot::database::tables {
using guest::Cpu;
using guest::compare;
using guest::compare_logical;

// TOC slots (offsets from r2): words that hold the address of a global.
namespace slot {
constexpr std::int32_t club_count = -0x7b50;       // number of clubs (the club table's size)
constexpr std::int32_t club_table = -0x7be8;       // word holding the club table's base
constexpr std::int32_t text_pool = -0x5618;        // the module's string pool (assert texts, format strings)
constexpr std::int32_t message = -0x7ba8;          // the shared message buffer
constexpr std::int32_t success = -0x7bac;          // the shared success flag (a byte)
constexpr std::int32_t result_buffer = -0x7bb8;    // the "Club table has %d entries" line's buffer
}
// A club record, 0x248 bytes, indexed by club id.
namespace club {
constexpr std::uint32_t stride = 0x248;
constexpr std::uint32_t id = 0x0, nation = 0x2c /* the record of the club's nation (inferred) */, rating = 0x1ec /* int16 */;
}
// The nation record: +0x68 its number (inferred), +0x120 a flag byte (inferred: the nation is played).
namespace nation {
constexpr std::uint32_t number = 0x68, flag = 0x120;
}

// The generation block's instruction helpers (loads into a register, extsb, srawi with XER[CA]); the hardware
// loads of a signed halfword and the multiply-high are not among them.
using generation::extsb;
using generation::load32;
using generation::load8;
using generation::srawi;
inline std::uint32_t load16_signed(Cpu& c, unsigned n, std::uint32_t address) {
    const auto value = guest::extended(c.i16(address));
    c.set(n, value);
    return value;
}
inline std::uint32_t mulhw(std::uint32_t a, std::uint32_t b) {
    return std::uint32_t(std::uint64_t(std::int64_t(std::int32_t(a)) * std::int64_t(std::int32_t(b))) >> 32);
}
}
