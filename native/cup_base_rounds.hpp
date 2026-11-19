#pragma once
// CUP.CPP, block `rounds` (cup_base_rounds*.cpp): a round's draw, its fixtures and their news. Routines of the
// block call each other directly; everything of other blocks and modules goes through the dispatcher.
// Dependencies still generated (dispatcher): the `draws` block's 0x1047fd00, 0x10480620, 0x104810b0 (called by
// 0x1047e980), NEWS.CPP's item routines 0x102bdf90/0x102cb4b0/0x102cc140/0x102cdbf0, FIX_MAN.CPP's 0x1048a2a0,
// 0x1048a140, 0x104888f0, 0x10738c00/0x10738e00/0x10738c20 (its iterator), 0x104aff20 (the fixture's rules),
// 0x102aed80, 0x10494d20, the virtuals +0x84 and +0x98 of the cup.
#include "guest_abi.hpp"

namespace cm::aot::cup_base::rounds {
using guest::Cpu;
// Makes the draw of a round whose match table is not drawn yet (byte +0x34 of the table's first tie = 0): moves
// a round's dates when a club is missing, orders and pairs the clubs, marks the ties and files the draw's news
// item 0x27. Returns 1.                                                                            // 0x1047e980
std::uint32_t make_round_draw(Cpu&, std::uint32_t cup, std::uint32_t round);
// The next round: draws it (when undrawn), schedules the fixtures of the current round's ties, makes the draw's
// news and moves the current round on. Returns 1, or -1 (an assertion) for a round outside the cup. // 0x1047ef50
std::uint32_t advance_round(Cpu&, std::uint32_t cup, std::uint32_t round);
// Schedules the fixtures of one tie (`tie`) of a round, or of the first tie waiting (`tie` = -1): `first_leg`
// is r6 when `tie` is given (1: the state-1 tie is only marked 2). Returns 0 when no tie is waiting. // 0x1047f3a0
std::uint32_t schedule_fixtures(Cpu&, std::uint32_t cup, std::uint32_t round, std::uint32_t tie, std::uint32_t first_leg);
// Puts each tie's higher-ranked / same-nation clubs apart: swaps a tie's sides (and the links into it) when
// the same club group is first in two ties of the round.                                              // 0x10485d70
void separate_same_nation_ties(Cpu&, std::uint32_t cup, std::uint32_t round);
// Orders the sides of ties from `first` up to (not including) `first + 1`, or of every tie when `first` is -1,
// by the three rules the round's flags choose (bit 1: virtual +0x98; bit 8: entrant state; else the cup's
// nation).                                                                                        // 0x10481ad0
void order_tie_sides(Cpu&, std::uint32_t cup, std::uint32_t round, std::uint32_t first);
// The draw's news items 0x14 and 0x03 for the fixtures of the round's date, reached by the iterator
// of FIX_MAN.                                                                                    // 0x10481e90
void draw_news(Cpu&, std::uint32_t cup, std::uint32_t date, std::uint32_t round);
// The news item 0x26 for a tie with a missing side (and the earlier ties' clubs it takes).         // 0x104860a0
void missing_side_news(Cpu&, std::uint32_t cup, std::uint32_t round, std::uint32_t tie);
}
