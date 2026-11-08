#pragma once
// MATCH_MAN.CPP match block (in-match team management): record offsets, the
// original's random source and the idioms the three routines share. Meanings
// are inferred from how the routines use the fields.
#include "guest_values.hpp"
#include "match_man_detail.hpp"
#include <cstdint>

namespace cm::aot::match_man {
// A match side (0xd8 bytes at match+0x7aac + side * 0xd8), the fields this block reads.
namespace lineup_side {
constexpr std::uint32_t match = 0x14;       // the match object
constexpr std::uint32_t manager_flag = 0x18;  // a byte: set for a human manager (inferred)
constexpr std::uint32_t substitutes = 0x19;   // a byte: substitutions still allowed (inferred)
constexpr std::uint32_t index = 0x1a;       // a byte: the side's number (0 or 1)
constexpr std::uint32_t kind = 0x1b;        // a byte
constexpr std::uint32_t mode = 0x1c;        // a byte
constexpr std::uint32_t manager = 0x20;     // handed to the position assignment
constexpr std::uint32_t coach = 0x24;       // staff record read for the pick's adjustments (0: none)
constexpr std::uint32_t players = 0x28;     // pointer to its 20 player records
constexpr std::uint32_t tactics = 0x30;     // the side's tactics object
constexpr std::uint32_t club = 0x34;
constexpr std::uint32_t opponent = 0x38;    // the other club
constexpr std::uint32_t extras = 0x3c;      // bench size nibble at +0x4e, mode byte at +0x3a, word at +0
constexpr std::uint32_t extras_bench = 0x4e, extras_mode = 0x3a;
}
// The match object.
namespace lineup_match {
constexpr std::uint32_t events_flag = 0x7931;  // a byte: events enabled
constexpr std::uint32_t minutes = 0x7948;      // 16-bit
constexpr std::uint32_t competition = 0x794c;  // 16-bit
constexpr std::uint32_t events = 0x79f4;       // the events object
constexpr std::uint32_t side_stride = 0x18e0, lineups = 0x7c6c;  // a lineup record per side
}
// A player record of a match (0x1c4 bytes; match_man_detail.hpp has its size and person).
namespace lineup_player {
constexpr std::uint32_t position = 0x19;   // a byte: >= 0 on the pitch, -1 bench, -2 and -4: off
constexpr std::uint32_t role = 0x1a;
constexpr std::uint32_t off_flag = 0x1e;   // a byte
constexpr std::uint32_t came_on = 0x1f;    // a byte: minute-based, capped by competition
constexpr std::uint32_t went_off = 0x20;   // a byte
constexpr std::uint32_t substituted = 0x22;  // a byte
constexpr std::uint32_t gone = 0x27;
constexpr std::uint32_t temper = 0x2f;     // a byte shown in the substitution events
constexpr std::uint32_t form = 0x32;       // 16-bit
constexpr std::uint32_t morale = 0x34;     // 16-bit
constexpr std::uint32_t fitness = 0x36;    // 16-bit
}
// The global at TOC -0x7a6c: the squad registry, its candidate table and count.
namespace registry {
constexpr std::uint32_t candidates = 0x0;                // table of 0x28-byte candidate records
constexpr std::uint32_t group_a = 0x5b8, group_b = 0x5c0;  // the counts added
constexpr std::uint32_t preferred = 0x8;                 // words indexed by the coach's preference (1..9)
}
// A candidate record (0x28 bytes).
namespace candidate {
constexpr std::uint32_t player = 0x0, value = 0x2;  // 16-bit: the player's id, a value
constexpr std::uint32_t weight_a = 0x10, weight_b = 0x18, weight_c = 0x20;  // doubles by formation mode
}
// The pool of doubles at TOC -0x5278 (match_man::slots::constants): offsets this block reads.
namespace pool {
constexpr std::uint32_t one = 0x0, half = 0x8;
constexpr std::uint32_t conversion = 0x90;  // 2^52 + 2^31
}
// Entries of other routines this block calls.
namespace lineup_entries {
constexpr std::uint32_t random_reseed = 0x1077b770, random_bounded = 0x10209d10;
constexpr std::uint32_t player_category = 0x10289560, player_role = 0x102895e0, benched_category = 0x102895a0;
constexpr std::uint32_t find_player = 0x102107c0, assign_positions = 0x10210c50, apply_assignment = 0x102104d0;
constexpr std::uint32_t pick = 0x1020d7d0, side_cleanup = 0x1020a290, events = 0x10201180;
constexpr std::uint32_t person_rating = 0x10113e10, person_form = 0x101149f0;
constexpr std::uint32_t competition_check = 0x100c7d00, squad_check = 0x1014c2e0;
constexpr std::uint32_t registry_check = 0x10147e70, registry_member = 0x10149df0;
}
// Dividing as the original does (mulhw magic, verified over the whole int32 range): C++ truncation.

// The game's random source: a cursor (a pointer into a table of halfwords) that
// moves two bytes on per draw, restarting and reseeding when it passes the limit,
// and a seed word.
struct RandomStream {
    std::uint32_t cursor_slot;    // TOC -0x7b84: address of the cursor word
    std::uint32_t seed_slot;      // TOC -0x7b90
    std::uint32_t limit_address;  // the word TOC -0x7b88 points to
    std::uint32_t restart;        // TOC -0x7b8c: the cursor's restart value
    std::uint32_t at(guest::Cpu& c) const { return c.u32(cursor_slot); }
    std::uint32_t draw(guest::Cpu& c) const { return c.u16(at(c)) + c.u32(seed_slot); }  // lhz + seed
};
// Every use of the stream's cursor: advance, then (past the limit) restart and reseed.
// `old_cursor_in_r4` mirrors a site that leaves the old cursor in r4.
inline void advance_stream(guest::Cpu& c, const RandomStream& stream, std::uint32_t reseed_back, bool old_cursor_in_r4 = false) {
    const auto old_cursor = c.u32(stream.cursor_slot);
    const auto limit = c.u32(stream.limit_address);
    const auto cursor = old_cursor + 2;
    if (old_cursor_in_r4) c.set(4, old_cursor);
    c.set(3, cursor);
    c.put32(stream.cursor_slot, cursor);
    if (guest::compare_logical(c, cursor, limit) > 0) {
        c.put32(stream.cursor_slot, stream.restart);
        c.call(lineup_entries::random_reseed, reseed_back);
        c.put32(stream.seed_slot, c.r(3) & 0xffff);
    }
}
// The int-to-double idiom with the original's fsub (not fsubs): the words
// 0x43300000 and value ^ 0x80000000 at `at`, loaded as a double, less the bias.
inline double converted(guest::Cpu& c, guest::Fpu& fpu, std::uint32_t at, std::int32_t value, double bias) {
    constexpr std::uint32_t exponent_word = 0x43300000;
    const std::uint32_t flipped = std::uint32_t(value) ^ 0x80000000u;
    c.put32(at, exponent_word);
    c.put32(at + 4, flipped);
    return fpu.fsub(std::bit_cast<double>(std::uint64_t(exponent_word) << 32 | flipped), bias);
}
// The stack temporaries one inlined draw uses (absolute addresses): n converted to a
// double, the seed's remainder converted, fctiwz's result (whose low word is read back).
struct SpreadSite {
    std::uint32_t n_at, rest_at, part_at;
    std::uint32_t bounded_back;  // the address after the site's bl to 0x10209d10
    bool limit_in_r4;            // the register schedule the site leaves after that call
    unsigned result_register;    // the register that keeps the result (r14 or r20)
};
// An inlined random(n) for n != 0 whose stream already advanced: the seed's remainder by n
// (negated for n <= 0) when |n| fits 16 bits; beyond that part = trunc(n / 65535.0 * (x mod
// 65535)) plus a tenth of a bounded draw (0x10209d10), capped at n - 1 (quirk: no lower bound).
// ef70 sites take 16-bit n only; the wide path is reached by 0x1020d7d0 with a huge count.
// Leaves the registers as the original does and returns the value (also in the result register).
std::int32_t random_spread(guest::Cpu& c, guest::Fpu& fpu, const RandomStream& stream, std::int32_t n,
                           std::uint32_t pool_address, const SpreadSite& site);
}
