#pragma once
// MATCH_ENG.CPP, rating block, part two: choosing the opposing player to meet a given player,
// small fixture tests and scans over a side's 20 players, and the freeing of the record tables.
#include "match_eng.hpp"

namespace cm::aot::match_eng {
// 0x101fce60: a fixture of kind 0x82 without a pending flag, whose tie totals for `argument` show a result and a zero fourth byte.
std::uint32_t kind_82_free(Cpu&, std::uint32_t match, std::uint32_t argument);
// 0x101fcf20: for a finished fixture the call 0x104a26b0 on the competition's entry; for kind 0x96 the same test as 0x101fce60.
std::uint32_t kind_96_free(Cpu&, std::uint32_t match, std::uint32_t argument);
// 0x101fd000: whether the player (a side or index byte) is flagged in the match's record at +0x7c6c + side * 0x18e0, or in position 3 or 4.
std::uint32_t side_flagged(Cpu&, std::uint32_t match, std::uint32_t side);
// 0x101fd100: the opposing player to meet `side`'s player `index`: the first whose flags pair with his
// (table of flag bits), else the one with the lowest score of 0x101ef8d0; -1 when the player is off the pitch or flagged.
std::uint32_t pick_opponent(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t index);
// 0x101fea10: 1 when 0x101eea30 accepts the ball cell for side 0, -1 when for side 1, else 0.
std::uint32_t ball_cell_side(Cpu&, std::uint32_t match);
// 0x101feae0: the fixture totals of 0x101f21d0 for `argument`: whether the first byte does not exceed the second, without the later ones.
std::uint32_t totals_not_ahead(Cpu&, std::uint32_t match, std::uint32_t argument);
// 0x101feba0: how many players of the other side than the one 0x10283690 names for `key` have 0x10285ef0 equal to it.
std::uint32_t count_matching_players(Cpu&, std::uint32_t match, std::uint32_t key);
// 0x101fec50: the first such player, or 0.
std::uint32_t first_matching_player(Cpu&, std::uint32_t match, std::uint32_t key);
// 0x101fed00: the first player of the side 0x10283690 names who is on the pitch, passes 0x10272100, fails 0x102720d0 and 0x101d7dc0, or 0.
std::uint32_t first_eligible_player(Cpu&, std::uint32_t match, std::uint32_t key);
// 0x101fedd0 and 0x101fee30: 0x101182a0 / 0x10118a40 on the match's record at +0x7c6c (flag 0) or +0x954c.
void apply_record_a(Cpu&, std::uint32_t match, std::uint32_t second);
void apply_record_b(Cpu&, std::uint32_t match, std::uint32_t second);
// 0x101fee90: frees the heap tables of the 0x2a5 records (0x14 bytes each) and resets two bytes of the data area.
void free_record_tables(Cpu&);
}
