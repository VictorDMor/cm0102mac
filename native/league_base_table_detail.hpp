#pragma once
// Block `table`'s private names: the layout of a standings row and of its head-to-head entries, the fixture
// record's fields the result routines read, and the pointer-glue call through a descriptor word. Meanings
// marked "(inferred)" are not confirmed by a reader outside this block.
#include "league_base_detail.hpp"
#include "league_base_table.hpp"

namespace cm::aot::league_base::table {
// A standings row (league::standing_row = 0x44 bytes). The league's word at +0xac selects a statistics group
// (0 totals, 1 home, 2 away; 0x10 bytes apart) for the comparators.
namespace row {
inline constexpr std::uint32_t
    club = 0x00,           // word: the club record (its first word is the club id; +0x3c a pointer, +0x1ee the name)
    position = 0x04,       // byte: the row's position, written by the sort
    head_to_head = 0x08,   // word: pointer to the club's head-to-head entries (0x10 bytes each, count-1 of them), or 0
    history = 0x0c,        // word: pointer to a byte array: the position after each match day (inferred)
    word_10 = 0x10,
    totals = 0x14,         // group 0: played, won, drawn, lost (bytes), scored, conceded, points (halfwords)
    home = 0x24,           // group 1
    away = 0x34,           // group 2
    status = 0x40,         // signed byte: -1 for a club still in the race (inferred)
    group_stride = 0x10,
    away_scored = 0x38;    // the away group's scored goals (tie-break 5)
// Fields inside a group.
inline constexpr std::uint32_t played = 0, won = 1, drawn = 2, lost = 3, scored = 4, conceded = 6, points = 8;
}
// A head-to-head entry (0x10 bytes) of a row's table at row::head_to_head.
namespace entry {
inline constexpr std::uint32_t club = 0, played = 4, won = 5, drawn = 6, lost = 7, scored = 8, conceded = 0xa, points = 0xc, size = 0x10;
}
// The tie-break codes of league::tie_breaks (inferred from the comparators): 1 goal difference, 2 goals
// scored, 3 wins, 4 the previous position (switches on the head-to-head pass), 5 away goals, 6 keep the order
// (only while the group is 0); 0 ends the list.
namespace tie_break {
inline constexpr std::int32_t goal_difference = 1, goals_scored = 2, wins = 3, position = 4, away_goals = 5, keep_order = 6;
}
// A match record (the fixture the result routines read; fixtures.cpp's 0x4c-byte rows are another class).
namespace fixture {
inline constexpr std::uint32_t
    home = 0x1c, away = 0x20,        // words: the clubs
    flags = 0x38,                    // halfword: bit 1 extra time played, bit 0x20 / bit 1 penalties (inferred)
    decider_home = 0x3d, decider_away = 0x3e,  // bytes the result sets to 3, 4 or 5 (inferred)
    competition = 0x42,              // signed byte: the stage index, -1 for the league itself (inferred)
    home_goals = 0x43, away_goals = 0x44,                // signed bytes
    extra_home = 0x47, extra_away = 0x48,                // signed bytes, extra time (-1 at +0x47: not played)
    penalties_home = 0x49, penalties_away = 0x4a;        // signed bytes
}
// League fields only this block reads.
namespace field {
inline constexpr std::uint32_t
    group = league::word_ac,         // word: the statistics group (above)
    rounds_played = 0xa6,            // halfword (inferred)
    played_limit = league::half_a4,  // halfword: matches a club plays (the sort fills it from rounds * (clubs - 1))
    compare = league::word_a0,       // word: the alternative comparator descriptor stored at TOC -0x3754
    ruling_flags = 0xa8,             // halfword (aa430 tests bits 1, 0x20, 1)
    win_points = league::win_points, draw_points = league::draw_points;
}
// Sign-extended loads, as `lha` / `extsb` leave them in a register.
inline std::uint32_t lha(Cpu& c, std::uint32_t address) { return std::uint32_t(std::int32_t(c.i16(address))); }
inline std::uint32_t lbz_extsb(Cpu& c, std::uint32_t address) { return std::uint32_t(std::int32_t(c.i8(address))); }
// `bl 0x10781320` with the descriptor in r12 (the word `lwz r12,-0x3754(r2)` loaded): the pointer glue saves r2
// at 0x14(r1) of the caller's frame and the caller reloads it afterwards. r3.. are set by the caller.
inline std::uint32_t call_descriptor(Cpu& c, std::uint32_t descriptor, std::uint32_t back) {
    c.set(12, descriptor);
    c.call(comp::callee::pointer_glue, back);
    c.set(2, c.u32(c.sp() + 0x14));
    return c.r(3);
}
// A word written to a TOC slot (`stw rX,slot(r2)`).
inline void put_toc(Cpu& c, std::int32_t slot, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(slot), value); }
}
