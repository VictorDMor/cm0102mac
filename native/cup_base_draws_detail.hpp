#pragma once
// Pieces the three round-order routines of block `draws` share (cup_base_draws_*.cpp).
#include "cup_base_detail.hpp"
#include "cup_base_draws.hpp"

namespace cm::aot::cup_base::draws::detail {
// A round's tie rows (0x36 bytes each, cup::round_matches[round]): the club slots at +0 (home) and +2 (away): -1
// empty, -2 to be drawn, else an index into the entrants; the placement a draw leaves.
namespace tie {
inline constexpr std::uint32_t home = 0, away = 2, back_slot = 6, back_leg = 8, back_round = 0x12,
                               home_first = 0xe, home_second = 0xa, away_first = 0x10, away_second = 0xc, size = 0x36;
}
// The gathered slots (0x10775130 new[] of 8 * the round's entrants bytes): a club slot, two words of its tie row
// (+0xe/+0x10 and +0xa/+0xc) and the club's state byte (entrants +4); -1 slots are 0xffff / 0xff.
namespace slot {
inline constexpr std::uint32_t team = 0, first = 2, second = 4, state = 6, size = 8;
}
inline constexpr std::uint32_t round_flags = 0xc, round_slots = 0x1a, round_ties = 0x1c, same_nation_flag = 4,
                               club_nation = 0x2c;
// divw's quotient, without the host's trap for the one overflowing pair (INT_MIN / -1; the result is undefined on the
// PowerPC and unreachable here: the dividend is a halfword plus the seed).
inline std::int32_t divide(std::int32_t dividend, std::int32_t divisor) {
    return divisor == -1 ? std::int32_t(0u - std::uint32_t(dividend)) : dividend / divisor;
}
// `lhax`: a halfword sign-extended into the register.
inline std::uint32_t half(Cpu& c, std::uint32_t at) { return sext16(c.u16(at)); }
// The sample's parity test (0x10480228..0x10480248): equal when the sample is even, whatever its sign.
inline bool even_sample(const Pool& pool) { return (pool.sample() & 1u) == 0; }

// The three routines' "Memory Allocation Error" path (0x1047fd7c..): the assertion's split path and version text,
// the format with the failing routine's text, the localized title and message, the error dialog, the success
// flag, exit. `at` is each call's own address (the return address is `at + 4`).
struct MemoryFailure {
    std::uint32_t split_at, format_at, title_at, message_at, dialog_at, exit_at;
    std::int32_t name, extension;
    std::uint32_t line, text;  // the source line (r8), the text's offset in the pool (r9)
    bool flag_in_r4 = false;   // the original loads the success flag's address into r4 to clear it
};
inline void memory_failure(Cpu& c, const Frame& frame, const MemoryFailure& f) {
    const auto pool = c.toc(toc::pool), message = c.toc(toc::message);
    c.set({{3, pool}, {4, 0}, {5, 0}, {6, frame.at(f.name)}, {7, frame.at(f.extension)}});
    c.call(comp::callee::split_path, f.split_at + 4);
    c.call(comp::callee::version_text, f.split_at + 8);
    c.set({{5, c.r(3)}, {3, message}, {4, pool + text::report_format_text}, {6, frame.at(f.name)},
           {7, frame.at(f.extension)}, {9, pool + f.text}, {8, f.line}});
    c.call(comp::callee::sprintf, f.format_at + 4);
    c.set({{3, c.toc(-0x79dc)}, {4, pool + text::memory_title}});
    c.call(comp::callee::format, f.title_at + 4);
    c.set({{3, c.toc(-0x79e0)}, {5, message}, {4, pool + text::memory_message}});
    c.call(comp::callee::format, f.message_at + 4);
    c.set({{3, c.toc(-0x79dc)}, {4, c.toc(-0x79e0)}, {5, 0}});
    c.call(comp::callee::error_dialog, f.dialog_at + 4);
    if (f.flag_in_r4) c.set(4, c.toc(-0x7bac));
    c.put8(c.toc(-0x7bac), 0);
    c.set(3, 0xffffffffu);
    c.call(callee::exit, f.exit_at + 4);
}
}
