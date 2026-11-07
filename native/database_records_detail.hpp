#pragma once
// DATABASE.CPP's records block: the TOC words and record offsets its routines
// share. Meanings marked "inferred" come from how the routines use the values.
#include "guest_abi.hpp"
#include "guest_values.hpp"
#include <cstdint>

namespace cm::aot::database::records {
// TOC slots (offsets from r2).
namespace slot {
constexpr std::int32_t text_pool = -0x5618;     // the module's string pool: file name, format, title
constexpr std::int32_t number_pool = -0x5614;   // the module's constant pool (doubles)
constexpr std::int32_t message = -0x7ba8;       // shared message buffer
constexpr std::int32_t success = -0x7bac;       // shared success flag (byte)
// Words that hold other words' addresses; the ones the index arithmetic
// subtracts and doubles (inferred: the limits of a numbered range).
constexpr std::int32_t range_high = -0x7b50, range_low = -0x7b04;
constexpr std::int32_t club_table = -0x7be8;    // address of the club table's base word
constexpr std::int32_t club_table_ready = -0x7b98;
constexpr std::int32_t club_count = -0x7be4;
constexpr std::int32_t contract_manager = -0x7b7c;
constexpr std::int32_t short_a = -0x7bc8, short_b = -0x7bdc, short_c = -0x7b14;
}
constexpr std::uint32_t club_stride = 0x248;
constexpr std::uint32_t club_contract_lookup = 0x10082f90;  // CONTRACT_MANAGER.CPP
constexpr std::uint32_t pack_colour = 0x1076bb20;           // three bytes to a 16-bit value (inferred)

// The game's assert macro at the call site `site` (the address of its first
// call). Every site reads the same pool strings and puts the two stack
// buffers at the frame offsets `name` and `extension`.
inline void assertion(guest::Cpu& c, const guest::Frame& frame, std::uint32_t site, std::uint32_t line,
                      std::uint32_t name, std::uint32_t extension) {
    const auto pool = c.toc(slot::text_pool);
    guest::report_assertion(c, {site, pool, pool + 0xd, pool + 0x1a, frame.at(name), frame.at(extension), line});
}

// The id slots of a record (0x100c7dd0 reads them, 0x100c82a0 fills them);
// the meanings are inferred from how the kinds use them: single id words
// and lists of three, five, seven and fifty ids.
namespace id_slot {
constexpr std::uint32_t word_80 = 0x80, list_84 = 0x84, word_90 = 0x90, word_94 = 0x94, list_98 = 0x98,
                        list_160 = 0x160, list_174 = 0x174, list_190 = 0x190;
constexpr std::int32_t list_84_count = 3, list_98_count = 0x32, list_160_count = 5, list_174_count = 7,
                       list_190_count = 3;
}

// What an assert path ends with: the success flag's address reloaded into r4
// where the original does so, r0 = 0 and the answer in r3.
inline void leave_failed(guest::Cpu& c, std::uint32_t answer, bool reload_flag = true) {
    if (reload_flag) c.set(4, c.toc(slot::success));
    c.set(0, 0);
    c.set(3, answer);
}
// The assert macro at `site`, then the failed answer: the shared tail of every
// accessor's out-of-range path. Returns the answer's value for `return`.
inline std::int32_t fail_assertion(guest::Cpu& c, const guest::Frame& frame, std::uint32_t site, std::uint32_t line,
                                   std::uint32_t name, std::uint32_t extension, std::uint32_t answer = 0,
                                   bool reload_flag = false) {
    assertion(c, frame, site, line, name, extension);
    leave_failed(c, answer, reload_flag);
    return std::int32_t(answer);
}

// Whether the signed word at `value_address` is below the range limit, high
// less twice low (or less low alone), as 0x100ca010, 0x100c82a0, 0x100d12d0
// and 0x100d3c40 test it: the words are read low, high, then the value, and the
// original's registers are left (r3 twice low when doubled, r4 the value, r0 the
// limit, then r3 and r0 = 0 or 1 as `extsb. r0,r3` leaves them, and cr0).
inline bool below_range_limit(guest::Cpu& c, std::uint32_t value_address, bool doubled = true) {
    const auto low_slot = c.toc(slot::range_low), high_slot = c.toc(slot::range_high);
    const auto low = c.u32(low_slot);
    const auto high = c.u32(high_slot);
    c.set(3, low);
    c.set(0, high);
    if (doubled) c.set(3, low << 1);
    const auto value = c.u32(value_address);
    c.set(4, value);
    const auto limit = high - (doubled ? low << 1 : low);
    c.set(0, limit);
    const bool below = guest::compare(c, std::int32_t(value), std::int32_t(limit)) < 0;
    c.set(3, below ? 0 : 1);
    c.set(0, below ? 0 : 1);
    guest::compare(c, below ? 0 : 1);
    return below;
}
}
