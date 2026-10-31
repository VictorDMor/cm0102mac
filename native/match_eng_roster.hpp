#pragma once
// MATCH_ENG.CPP, roster: which player stands in which pitch cell, the goalkeepers,
// substitutions and the match's score bookkeeping. Original entries of the roster
// block. Cells are 0x5c bytes (two lists of 11 players, 0x2c bytes a side, and
// a count byte per side at +0x58) in a grid at +0xa08: row * 0x450 + column * 0x5c.
#include "match_eng.hpp"

namespace cm::aot::match_eng {
// Compares into cr0, as the original's branches do, and answers them.
inline bool cmp_eq(Cpu& c, std::int32_t a, std::int32_t b = 0) { return compare(c, a, b) == 0; }
inline bool cmp_ne(Cpu& c, std::int32_t a, std::int32_t b = 0) { return compare(c, a, b) != 0; }
inline bool cmp_lt(Cpu& c, std::int32_t a, std::int32_t b = 0) { return compare(c, a, b) < 0; }
inline bool cmp_le(Cpu& c, std::int32_t a, std::int32_t b = 0) { return compare(c, a, b) <= 0; }
inline bool cmp_gt(Cpu& c, std::int32_t a, std::int32_t b = 0) { return compare(c, a, b) > 0; }
inline bool cmp_ge(Cpu& c, std::int32_t a, std::int32_t b = 0) { return compare(c, a, b) >= 0; }
inline bool cmpl_eq(Cpu& c, std::uint32_t a, std::uint32_t b = 0) { return compare_logical(c, a, b) == 0; }
inline bool cmpl_ne(Cpu& c, std::uint32_t a, std::uint32_t b = 0) { return compare_logical(c, a, b) != 0; }

// 0x101f1530: a player takes the pitch (or comes back, `carried` != 0): his minutes
// and appearances, limited by the period's end, then out of his cell, the goalkeeper
// bookkeeping and the side's substitution clock.
void player_arrives(Cpu&, std::uint32_t match, std::uint32_t player, std::uint32_t carried);
// 0x101f1880: puts the player on the off-pitch cell (row 8, column 6) and in that cell's list.
void put_off_pitch(Cpu&, std::uint32_t match, std::uint32_t player);
// 0x101f1950: takes the player out of his cell and marker lists; when he held a
// goalkeeper's place (or `mode` is -3) the best other player of his side inherits his position.
void leave_cell(Cpu&, std::uint32_t match, std::uint32_t player, std::uint32_t mode);
// 0x101f1e10: the side's goalkeeper, found again among its players when the stored one is gone.
std::uint32_t side_goalkeeper(Cpu&, std::uint32_t match, std::uint32_t second_side);
// 0x101f21d0: the sides' running totals into the five bytes, and the tie's result.
std::uint32_t tie_totals(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t home_at, std::uint32_t away_at,
                         std::uint32_t decider_at, std::uint32_t two_legs_at, std::uint32_t knockout_at,
                         std::uint32_t leg_flag);
// 0x101f2400: closes a period's statistics (`mode` 0x3de, 0 or 1; anything else only refreshes the shares).
void period_statistics(Cpu&, std::uint32_t match, std::uint32_t mode);
// 0x101f3480: the possession-style shares (percent and its complement) of three counter pairs.
void share_percentages(Cpu&, std::uint32_t match, std::uint32_t first_kind);
// 0x101f3a30: the best player of the side for the kind of action (0..8), judged by a score; `excluded` is
// an optional list of 20 bytes per side of players to skip for kind 5.
std::uint32_t pick_player(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t kind, std::uint32_t excluded);
// 0x101f4860: adds an amount to the rating of each of a side's players on the pitch.
void adjust_side_ratings(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t amount);
// 0x101f4900: counts an event in the period and ring statistics.
void count_event(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t flag);
}
