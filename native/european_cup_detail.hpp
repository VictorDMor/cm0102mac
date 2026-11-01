#pragma once
#include "european_cup.hpp"
#include "guest_values.hpp"

namespace cm::aot::european_cup::detail {
using namespace guest;
constexpr std::uint32_t club_size = 0x248, definition_size = 0x6c, round_size = 0x78, refill = 0x1077b770;
namespace slot {
constexpr int pool = -0x45c8, doubles = -0x45cc, vtable = -0x6094, compare_seeds = -0x6098, compare_draw = -0x609c,
    flag = -0x7bac, message = -0x7ba8, text = -0x7bb8, definitions = -0x7bbc, clubs = -0x7be8, club_total = -0x7b50,
    history = -0x7b48, competitions = -0x7b5c, uefa_cup = -0x7a4c, this_competition = -0x7a30, reputation = -0x7a48,
    finances = -0x7ab8, nation_tables = -0x7aa4, rng_seed = -0x7b90, rng_start = -0x7b8c, rng_end = -0x7b88, rng_cursor = -0x7b84;
}
// Strings at the source pool, after the file name, version format and title.
namespace pool_text {
constexpr unsigned format = 0x11, title = 0x1e, qualified = 0x24, entered_uefa = 0x8a, out_phase_1 = 0xe4, out_phase_2 = 0x161,
    knocked_out = 0x1de, qualified_phase_1 = 0x23e, empty = 0x264, phase_1 = 0x265, qualified_phase_2 = 0x279, phase_2 = 0x29f,
    winners = 0x2b3, runners_up = 0x2bb, third = 0x2c6;
}
struct Competition {
    Cpu& c;
    std::uint32_t address;
    Field<std::uint32_t> definition() const { return {c, address + 4}; }
    Field<std::uint32_t> fixtures() const { return {c, address + 8}; }
    Field<std::uint32_t> stages() const { return {c, address + 0xc}; }
    Field<std::uint32_t> entrants() const { return {c, address + 0x14}; }
    Field<std::int32_t> stage_count() const { return {c, address + 0x2c}; }
    Field<std::int32_t> last_stage() const { return {c, address + 0x30}; }
    Field<std::int16_t> entrant_count() const { return {c, address + 0x36}; }
    Field<std::int16_t> entrant_scan() const { return {c, address + 0x38}; }
    Field<std::int16_t> extra_rounds() const { return {c, address + 0x3a}; }
    Field<std::int16_t> round_count() const { return {c, address + 0x3c}; }
    Field<std::int16_t> qualifier_count() const { return {c, address + 0x3e}; }
    Field<std::int16_t> year() const { return {c, address + 0x40}; }
    Field<std::uint32_t> rounds() const { return {c, address + 0xa0}; }
    Field<std::uint32_t> qualifiers() const { return {c, address + 0xa4}; }
    Field<std::uint32_t> group_tables() const { return {c, address + 0xac}; }
};
struct Club {
    Cpu& c;
    std::uint32_t address;
    Field<std::int32_t> competition() const { return {c, address + 0x14}; }
    Field<std::uint32_t> nation() const { return {c, address + 0x2c}; }
    Field<std::int16_t> reputation() const { return {c, address + 0x1ec}; }
    Field<std::int8_t> tier() const { return {c, address + 0x243}; }
};
struct Program {
    Cpu& c;
    std::uint32_t call(std::uint32_t target, std::uint32_t back,
                       std::initializer_list<std::uint32_t> args = {}) const {
        unsigned n = 3;
        for (const auto value : args) c.set(n++, value);
        c.call(target, back);
        return c.r(3);
    }
    std::uint32_t virtual_call(std::uint32_t object, unsigned offset, std::uint32_t back) const {
        c.set(12, c.u32(c.u32(object) + offset));
        c.call(0x10781320, back);
        c.set(2, c.u32(c.sp() + 0x14));
        return c.r(3);
    }
};
inline void assertion(Cpu& c, const Frame& f, std::uint32_t at, unsigned name, unsigned extension, unsigned line) {
    const auto pool = c.toc(slot::pool);
    report_assertion(c, {at, pool, pool + pool_text::format, pool + pool_text::title, f.at(name), f.at(extension), line});
}
// The expansion that reloads the pool and buffer from the TOC at each use:
// the same calls, with the last two four instructions further on.
inline void reloading_assertion(Cpu& c, const Frame& f, std::uint32_t at, unsigned name, unsigned extension, unsigned line) {
    const auto pool = c.toc(slot::pool);
    report_assertion(c, {at, pool, pool + pool_text::format, pool + pool_text::title, f.at(name), f.at(extension), line},
                     [&](std::uint32_t entry, std::uint32_t back) {
        c.call(entry, back == at + 0x24 ? at + 0x28 : back == at + 0x34 ? at + 0x3c : back);
    });
    c.set(3, c.toc(slot::flag));
}
// The shared random stream: a cursor walking a table of 16-bit samples, plus
// a seed the refill replaces when the cursor passes the table's end.
struct Stream {
    Cpu& c;
    std::uint32_t cursor, end, start, seed;
    void advance(std::uint32_t back) const {
        const auto next = c.u32(cursor) + 2;
        c.set(3, next);
        c.put32(cursor, next);
        if (compare_logical(c, next, c.u32(end)) > 0) {
            c.put32(cursor, start);
            c.call(refill, back);
            c.put32(seed, c.r(3) & 0xffff);
        }
    }
    std::uint32_t sample() const { return std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed); }
};
inline std::uint32_t signed_byte(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int8_t(value))); }
// `x % 2^bits` for a signed x, by the rotate sequence the compiler emits;
// leaves the sign bit in r3, as that sequence does.
inline std::int32_t signed_remainder(Cpu& c, std::uint32_t sum, unsigned bits) {
    const auto sign = sum >> 31;
    c.set(3, sign);
    return std::int32_t(std::rotl((sum << (32 - bits)) - sign, int(bits)) + sign);
}
// `srawi x,3; addze`: division by 8 rounding toward zero.
inline std::int32_t over_eight(std::uint32_t x) { return std::int32_t(x) / 8; }
inline std::uint32_t extended_half(std::uint32_t value) { return extended(std::int16_t(value)); }
inline void copy_bytes(Cpu& c, std::uint32_t to, std::uint32_t from) {
    c.put8(to + 4, c.u8(from + 4)); c.put8(to + 5, c.u8(from + 5)); c.put8(to + 6, c.u8(from + 6));
}
}
