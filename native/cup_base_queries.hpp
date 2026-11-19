#pragma once
// CUP.CPP, block `queries` (tools/cup_audit.py): the news a cup files, the prize money it pays, the rank it gives its
// clubs, and the small virtuals and queries on its rounds and match rows. Included at the end of cup_base.hpp.
//
// A cup's rounds are 0x78-byte rows (cup::rounds), its entrants 8-byte rows (+0 the club, +4 and +5 bytes), and the
// match rows of round k (cup::round_matches[k]) 0x36 bytes each: +0 and +2 the two sides' entrant indexes (-1: not
// decided yet; then the feeder tie of side 0 is round +0xe, tie +0xa, and of side 1 round +0x10, tie +0xc).
//
// Dependencies still generated or owned by other blocks (called through the dispatcher): the news manager's item
// routines (NEWS.CPP 0x102bdf90 start, 0x102cb4b0 set field, 0x102cc030/0x102cc140/0x102cc9e0 file, 0x102cdbf0 is
// asked by COMP's items), 0x10494d20 (FRIENDLY.CPP), 0x10464900/0x10463be0 (COMP_UTIL.CPP), the competitions'
// virtuals +0x4c/+0x70/+0x90, STAFF_RECORDS.CPP 0x10383410/0x10383b70/0x10384020/0x10384500/0x103846e0, KEY_NATION.CPP
// 0x10192130, COMP_STATS.CPP 0x10459b50/0x10457490, HOST_COUNTRY.CPP 0x1049d1c0, DATABASE.CPP 0x100c76c0, COMP_UTIL.CPP
// 0x10463990/0x10477b40, UEFA_SEEDING.CPP 0x104b7860, FINANCE.CPP 0x10103510/0x100f71f0, the allocator, the
// assertion report and the localized dialog. COMP.CPP's prize item (comp::news_prize_money) is called directly.
#include "guest_abi.hpp"

namespace cm::aot::cup_base::queries {
using guest::Cpu;
// (vt +0x48) News item 0x10 for a result (`club`, `kind`; `fixture` the played fixture, whose +0x34 is the round, +0x30
// the competition's code and +0x2c its holder; `argument_6` and `argument_8` are handed to the virtual +0x4c). Returns
// 0 when the fixture rules (0x10494d20) refuse, the virtual +0x4c answers -1 or `fixture` is null, else 1.
std::uint32_t file_result_news(Cpu&, std::uint32_t cup, std::uint32_t club, std::uint32_t kind, std::uint32_t argument_6,
                               std::uint32_t fixture, std::uint32_t argument_8);                                // 0x1047b360
// (vt +0x4c) Records a place with the staff-records object by `kind` (1 winner, 2, 3, 0 a round reached, -1); for a
// cup that is its own competition. Returns 0.
std::uint32_t record_place(Cpu&, std::uint32_t cup, std::uint32_t club, std::uint32_t kind, std::uint32_t fixture,
                           std::uint32_t argument_9);                                                           // 0x1047b5b0
// (vt +0x38) The clubs whose entrant byte +5 is 1, 2 and 3 (winner, runner-up, third; in the cup whose record id is the word at
// TOC -0x6af4 the third is instead the first entrant COMP_STATS gives state 3), and the club row of the host nation (0x1049d1c0 after
// the virtual +0x90, 0x100c76c0) are handed to 0x10463990; then, for a competition a nation descriptor names (0x10494d20 not
// refusing, 0x10192130), 0x1047bde0 runs.
void record_final_places(Cpu&, std::uint32_t cup);                                                              // 0x1047b7d0
// The index of the tie in `fixture`'s round (+0x34) one of whose sides is the club whose id is the word `fixture`+0xc;
// -1 when none; a null fixture asserts (line 0x387) and answers -1.
std::int32_t tie_index(Cpu&, std::uint32_t cup, std::uint32_t fixture);                                         // 0x1047ba50
// UEFA seeding: the first two clubs outside the final's tie among the semi-final round's ties go to 0x104b7860.
void file_uefa_seeding(Cpu&, std::uint32_t cup);                                                                // 0x1047bde0
// 0x10484b90 (called by COMP_SCREENS.CPP): the entrants' indexes that make up side (`first_side` byte: not 0 the first side, 0 the second) of tie
// `tie` of round `round` (round 0..rounds; an unfinished side expands through its feeder ties), as halfwords into `out`;
// the count is also the word at TOC -0x3764, and is returned. Asserts (lines 0x153c, 0x1541, 0x1546, 0x1571) and returns
// 0 for a round out of range, a cup without entrants, a tie of -1 or no entrant found.
std::uint32_t side_entrants(Cpu&, std::uint32_t cup, std::int32_t round, std::int32_t tie, std::uint32_t first_side,
                            std::uint32_t out);                                                                 // 0x10484b90
// 0x10484e10 (its recursion): appends the indexes of both sides of tie `tie` of round `round` to `out`, counting in the
// word `counter`; returns the count. Asserts (0x157e a null out or counter, 0x1584 a negative round or tie -1).
std::uint32_t append_entrants(Cpu&, std::uint32_t cup, std::int32_t round, std::int32_t tie, std::uint32_t out,
                              std::uint32_t counter);                                                           // 0x10484e10
// 0x10484fd0 (called by process_fixture): tells the cup (virtual +0x70) the three place bytes of entrant `entrant` after
// round `round`, by the entrant's byte +5 and the round's position among the cup's rounds.
void record_entrant_places(Cpu&, std::uint32_t cup, std::uint32_t entrant, std::uint32_t round);              // 0x10484fd0
// 0x10485340 (called by process_fixture): or-s the round's flags (+0x10) into the match's halfword +0x38 and, when bit
// 0x800 is then set, replaces it by 0x1000 (side 0's entrant state smaller), 0x800 (larger) or 1 (equal).
void settle_match_flags(Cpu&, std::uint32_t cup, std::uint32_t match, std::uint32_t tie);                      // 0x10485340
// (vt +0x5c) The round difference of `match`: cup byte +0x4b minus the rounds left (at least 1) is returned; when the match's
// byte +0x4b is not -1 `out` gets +0x4c minus +0x4b (+1; reversed when `reverse` is not 0), or -1 when that is below 1.
std::int32_t round_difference(Cpu&, std::uint32_t cup, std::uint32_t match, std::uint32_t reverse, std::uint32_t out);  // 0x104858b0
// 0x10485930 (called by process_fixture): pays the prize money of the round `round`'s tie to the winner's entrant row
// (`winner`) and the loser's (`loser`): files COMP's prize item, adds the amount to the club's cash (FINANCE) and takes
// it off the cup's pool (+0x28). The amounts are the round's (+0x70, +0x74) or a share of the pool.
void pay_prizes(Cpu&, std::uint32_t cup, std::uint32_t winner, std::uint32_t loser, std::uint32_t round, std::uint32_t fixture);  // 0x10485930
// (vt +0x98) The one of two fixtures whose competition record (+0x30; its halfword +0x18) is smaller; 0 when equal.
std::uint32_t lower_record(Cpu&, std::uint32_t cup, std::uint32_t first, std::uint32_t second);               // 0x10485ab0
// (vt +0x64, `CUP::rank_teams`) Gives the entrants, sorted by 0x10477b40, their rank in the statistics object (+8, 0x10457490),
// the last round's clubs first; asserts (0x1819 no statistics) or ends the program (an allocation failure).
void rank_teams(Cpu&, std::uint32_t cup);                                                                       // 0x10485b10
// (vt +0x8c) Tells the cup (virtual +0x70) the places 1, 1 and the club count's low byte; returns 1.
std::uint32_t record_group_places(Cpu&, std::uint32_t cup, std::uint32_t club);                                // 0x10486480
}
