#pragma once
// INJURY.CPP, block "onset": layouts and helpers its files share.
#include "guest_fpu.hpp"
#include "injury_unit_detail.hpp"
#include <cstdint>

namespace cm::aot::injury_unit::onset {
using guest::Cpu;
using guest::Fpu;
using guest::Frame;

// A person's status row (injury_unit_detail.hpp's status:: has the fields other blocks use).
namespace row {
inline constexpr std::uint32_t injury = 0x10,          // index of the injury record, -1 without one
                               news_list = 0x18,       // the two list heads the record's removal updates
                               news_list_end = 0x1c,
                               pending_type = 0x14,    // signed byte: the injury waiting to happen, -1 for none
                               pending_count = 0x15,   // how many times it was announced (byte)
                               cleared = 0x20,         // byte set to 0 whenever a new injury is registered
                               last_type = 0x21,       // byte: the type of the last registration
                               form = 0x22;            // signed byte: what the daily pass weighs against the draw
}
// An injury record (0x38 bytes; injury_unit_detail.hpp's injury:: has the rest).
namespace record {
inline constexpr std::uint32_t type = 0x00,            // signed byte, -1 in a free record
                               match_snapshot = 0x04,  // 0x1c bytes copied from the match the injury happened in
                               match_found = 0x20,     // signed byte, -1 when the injury did not happen in a match
                               started = 0x28,         // the date: halfword +0, halfword +2, byte +4
                               days_out = 0x30, days_unfit = 0x32, countdown_a = 0x34, countdown_b = 0x36;
}
// A row of the injury type table (0xc bytes; the game's data).
namespace type_row {
inline constexpr std::uint32_t klass = 1,       // byte, what 0x1018c180 answers
                               base_days = 4,   // signed halfword added to the length
                               spread = 6,      // signed halfword: scales the length from the player's skill
                               unfit_ratio = 8, // signed byte: days_unfit as a share of the length
                               pending = 9;     // signed byte, zero for types that cannot be pending
}
// A person record (0x70 bytes; the fields this block reads).
namespace person {
inline constexpr std::uint32_t index = 0x00, profile = 0x14, news_person = 0x20, club = 0x24,
                               relative_a = 0x28, relative_b = 0x2c, relative_c = 0x30,
                               news_a = 0x34, news_b = 0x36, morale = 0x5c, job = 0x62;
}
// The profile a person record points to.
namespace profile {
inline constexpr std::uint32_t standing = 0x0c,   // signed halfword
                               level = 0x0f,      // byte
                               fitness_a = 0x1b,  // bytes the daily pass wears down
                               fitness_b = 0x1d,
                               proneness = 0x2d,  // signed byte
                               resilience = 0x34, // signed byte
                               fitness_c = 0x36;
}
// The fitness table: 100-byte rows by person index, byte +10 counts the injuries a person picked up.
inline constexpr std::uint32_t fitness_row_size = 100, fitness_injuries = 10;

namespace toc_cell {
inline constexpr std::int32_t fitness_rows = -0x7b9c,     // the cell holding the fitness table
                              messages_object = -0x7a7c,  // the object 0x103d0990 tells about new injuries
                              career_object = -0x7b40,    // the career object (+0x988: the club's manager state)
                              press_object = -0x7a64,     // the object 0x102b8460 tells
                              alert_object = -0x7b28,     // the object 0x102de470 tells
                              league_object = -0x7b24,    // the object 0x10003dc0 asks
                              news_object = -0x7bf0,      // the news the injury news is added to
                              clubs_cell = -0x7be8, owner_object = -0x7b7c, owner_cell = -0x7b78,
                              season_date = -0x7a9c, jump_table_pick = -0x5440, jump_table_news = -0x545c,
                              spare_value = -0x7bb8;      // the first argument of the type-range message
}

// The inline "random below `range`" the compiler expanded at each of this block's sites: nothing for a
// zero range; otherwise one sample from the shared table, its remainder for a range of at most 0xffff
// either way (negated for a negative range), and for a larger range a scaled sample: the sample's
// remainder modulo 0xffff over 0xffff scaled by the range as doubles (the ratio of the range to the pool's
// first double, via fctiwz), plus a tenth of one more draw below range*10/0xffff (0x101864a0), added up
// to range-1. `after_redraw` and `after_scale` are the return addresses of the site's two calls;
// `result_register` is the register the original holds the (intermediate, then final) result in, which
// the callees save into their frames: it is set to the whole part before the nested call.
struct Site { std::uint32_t after_redraw, after_scale; unsigned result_register; };
inline std::int32_t bounded_draw(Cpu& c, std::int32_t range, const Site& site) {
    if (range == 0) {
        c.set(site.result_register, 0);
        return 0;
    }
    const auto sample = std::int32_t(draw(c, site.after_redraw));
    std::uint32_t result;
    if (range >= -0xffff && range <= 0xffff) {
        // divw in 64 bits, wrapping: defined for any word in the base cell (0x80000000 / -1 too).
        const auto quotient = std::uint32_t(std::int64_t(sample) / range);
        const auto remainder = std::uint32_t(sample) - quotient * std::uint32_t(range);
        result = range > 0 ? remainder : 0u - remainder;
    } else {
        Fpu fpu(c);
        const auto doubles = c.toc(toc::floats);
        const auto to_double = [&](std::uint32_t value) {
            const auto bits = (std::uint64_t(0x43300000) << 32) | (value ^ 0x80000000u);
            return fpu.fsub(std::bit_cast<double>(bits), c.f64(doubles + 8));
        };
        const auto ratio = fpu.fdiv(to_double(std::uint32_t(range)), c.f64(doubles));
        const auto tenth = std::int32_t(std::uint32_t(range) * 10u) / 0xffff;
        const auto remainder = sample - (sample / 0xffff) * 0xffff;
        const auto scaled = fpu.fmul(ratio, to_double(std::uint32_t(remainder)));
        const auto whole = std::uint32_t(fpu.fctiwz(0, scaled));
        c.set(site.result_register, whole);
        const auto extra = std::int32_t(invoke_unit(c, 0x101864a0, site.after_scale, {std::uint32_t(tenth)}) + 5u) / 10;
        result = whole + std::uint32_t(extra);
        if (std::int32_t(result) > std::int32_t(std::uint32_t(range) - 1u)) result = std::uint32_t(range) - 1u;
    }
    c.set(site.result_register, result);
    return std::int32_t(result);
}
// One sample taken as the compiler expanded `sample % divisor` for a positive constant.
inline std::uint32_t draw_remainder(Cpu& c, std::uint32_t after_redraw, std::uint32_t divisor) {
    // Signed, as the original's mulhw sequence: the sample is the base cell's word plus a halfword.
    return std::uint32_t(std::int32_t(draw(c, after_redraw)) % std::int32_t(divisor));
}
}
