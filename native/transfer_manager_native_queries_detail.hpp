#pragma once
// Shared pieces of the "queries" block's routines (TRANSFER_MANAGER.CPP): iterator calls, the offer walk,
// the nation rule table, dates and the original's date comparison.
#include "guest_fpu.hpp"
#include "transfer_manager_native_detail.hpp"
#include <bit>
#include <initializer_list>

namespace cm::aot::transfer_manager_native::queries_detail {
using guest::Cpu;
using guest::Frame;
constexpr std::uint32_t iterator_construct = 0x10738c00, iterator_destroy = 0x10738c20, iterator_next = 0x10738e00,
                        offers_of_person = 0x103eaea0, has_offers = 0x103f5e50, find_contract = 0x10082f20,
                        call_through_vector = 0x10781320;
constexpr std::uint32_t iterator_slot = 0x38, iterator_cursor_from = 0x3c, iterator_cursor = 0x44;
constexpr std::uint32_t status_rows = 0x9a0, status_row_size = 10, status_row_flags = 0;
constexpr std::uint32_t offer_kind = 0x34, offer_club = 8;
constexpr std::uint32_t pool_magic = 8, contract_type_byte = 0x38, contract_club = 4;
constexpr std::uint32_t nation_rules = 0x9e0, nation_count = 27, nation_key = 0xc, club_nation = 0x2c;
constexpr std::int32_t people_count_cell = -0x7be4, pool_cell = -0x4ba0, now_cell = -0x7b80;

inline void destroy_iterator(Cpu& c, std::uint32_t iterator, std::uint32_t back) {
    invoke(c, iterator_destroy, back, {iterator, 0xffffffffu});
}

// The walk over a person's offers: 0x103eaea0 fills the iterator, then the cursor word it left is copied to the
// slot that 0x10738e00 steps from.
inline void start_offer_walk(Cpu& c, const Frame& frame, std::uint32_t manager, std::uint32_t person, std::uint32_t flag,
                             std::uint32_t back) {
    invoke(c, offers_of_person, back, {manager, person, frame.at(iterator_slot), flag});
    c.put32(frame.at(iterator_cursor), c.u32(frame.at(iterator_cursor_from)));
}

// The offer a walk step names: the manager's offer array (word 0) at the step's index, or its first entry
// when the index is beyond the count (word 1).
inline std::uint32_t offer_of_step(Cpu& c, std::uint32_t manager, std::uint32_t step) {
    const auto index = c.i32(step);
    const auto list = c.u32(manager);
    return c.u32(index < c.i32(manager + 4) ? list + std::uint32_t(index) * 4 : list);
}

// The status table row (10 bytes) of a person id.
inline std::uint32_t status_row(Cpu& c, std::uint32_t manager, std::uint32_t id) {
    return c.u32(manager + status_rows) + id * status_row_size;
}

// The original's int-to-double: the magic exponent word with the sign-flipped integer, less the pool constant.
inline double to_double(guest::Fpu& fpu, Cpu& c, std::uint32_t pool, std::int32_t x) {
    return fpu.fsub(std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | (std::uint32_t(x) ^ 0x80000000u)),
                    c.f64(pool + pool_magic));
}

// The club table entry of the club a found contract is with; 0 unless the contract exists and is not of type 5.
inline std::uint32_t contract_club_of(Cpu& c, std::uint32_t contract) {
    if (contract == 0 || (c.u8(contract + contract_type_byte) >> 2) == 5) return 0;
    return c.u32(c.toc(toc::clubs)) + c.u32(c.u32(c.toc(toc::found_contract)) + contract_club) * club_size;
}

// The nation rule record whose key is `nation` among the manager's 27; 0 when none.
inline std::uint32_t nation_rule(Cpu& c, std::uint32_t manager, std::uint32_t nation) {
    for (std::uint32_t index = 0; index < nation_count; ++index) {
        const auto rule = c.u32(manager + nation_rules + index * 4);
        if (c.u32(rule + nation_key) == nation) return rule;
    }
    return 0;
}

// A virtual call of a nation rule record through the vector bridge: `slot` of its table, arguments in r3...,
// the table of contents restored from the frame afterwards as the original does.
inline std::uint32_t call_rule(Cpu& c, const Frame& frame, std::uint32_t rule, std::uint32_t slot, std::uint32_t back,
                               std::initializer_list<std::uint32_t> arguments) {
    unsigned n = 3;
    for (const auto argument : arguments) c.set(n++, argument);
    c.set(12, c.u32(c.u32(rule) + slot));
    c.call(call_through_vector, back);
    c.set(2, c.u32(frame.at(0x14)));
    return c.r(3);
}

// A date as the game keeps it (5 bytes): the day in a signed half, the year in an unsigned half, one more byte.
struct Date { std::int32_t day; std::uint32_t year, extra; };
inline Date read_date(Cpu& c, std::uint32_t at) { return {c.i16(at), c.u16(at + 2), c.u8(at + 4)}; }
inline void write_date(Cpu& c, std::uint32_t at, const Date& date) {
    c.put16(at + 2, date.year);
    c.put16(at, std::uint32_t(date.day));
    c.put8(at + 4, date.extra);
}

// The original's date comparison "a is later than b": years as unsigned halves, then the days through its
// shift-and-subtract sign trick.
inline bool later_than(std::uint32_t a_year, std::int32_t a_day, std::uint32_t b_year, std::int32_t b_day) {
    if (a_year > b_year) return true;
    if (a_year < b_year) return false;
    const auto x = std::uint32_t(a_day ^ b_day);
    const auto difference = std::uint32_t(std::int32_t(x) >> 1) - (x & std::uint32_t(a_day));
    return (difference >> 31) != 0;
}

// A read of a frame word the original makes without writing it first (a stale read of the stack
// contract). Its value is unused, so the load is free under the per-reader contract; it is kept so
// that `cm-world-replay --stack-audit` still sees the original's reader while the C++ runs.
inline void read_stale(Cpu& c, std::uint32_t at) {
    const auto value = c.u32(at);
    __asm__ volatile("" : : "r"(value));
}
}
