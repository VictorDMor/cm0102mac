#pragma once
// CUP.CPP, block `daily` (tools/cup_audit.py): the cup's daily update, the question whether it is finished, a round's
// calendar entries and the small decisions on a played tie. Every entry returns what the original returns and leaves
// r4-r10 as the original does (callers read them: 0x10485410's loop bound in r4, the calendars' r3-r5 after the
// setters). Dependencies (other modules, called through the dispatcher unless a typed header is named): the virtuals
// of the stages (+0x18, +0x1c) and of the cup itself (+0x18, +0x1c, +0x30, +0x38), 0x1000cfc0 (the object at TOC
// -0x7ad4), COMP_STATS 0x10456290 / 0x104573c0 (the competition's statistics object, +0x8), 0x1047e980 and
// 0x1047ef50 (block `rounds`, a round's draw and the advance to the next round), FIX_MAN.CPP 0x1048aea0 (a fixture
// lookup, still generated), the allocator-free assertion report (0x10739890, 0x10768810, 0x1077b720, 0x10769b60),
// exit 0x1076f7d0; DATE.CPP through date.hpp (0x107156b0, 0x107156f0, 0x10717890, 0x10717b20, 0x10717cb0).
#include "guest_abi.hpp"

namespace cm::aot::cup_base::daily {
using guest::Cpu;
// 0x10482ca0 (vt +0x1c, `when` the date word the day arrives with, `phase` the byte in r5: 0 the morning pass, else the
// evening pass; the generators' hook returns 0 for the retired Inter-American Cup first): always 0. The stack
// readers: GAME.CPP 0x1011d1d4 reads what DATE 0x107156b0 leaves at the call of 0x10483034, the routine's own readers
// at 0x10482f40 and 0x104830e8 read its prologue image and what 0x10485410 left.
std::uint32_t daily_update(Cpu&, std::uint32_t cup, std::uint32_t when, std::uint32_t phase);
// 0x10485410 (vt +0x18): whether the cup is finished (byte +0xb0 set; with `mode` 1 and a competition that is its
// own, every stage must be finished too and the last must have been reached).
std::uint32_t finished(Cpu&, std::uint32_t cup, std::uint32_t mode);
// 0x10482640 (vt +0xa4) (inferred): a tie's outcome from its two sides' figures (`tie`, kind byte +0x33) and the
// fixture's flags (+0x38): the winning side 1 or 2, 3 for a first leg left open, 4 undecided; it writes the
// fixture's two outcome bytes (+0x3d, +0x3e). An unreachable tie of the flags reports an assertion.
std::uint32_t tie_outcome(Cpu&, std::uint32_t cup, std::uint32_t tie, std::uint32_t fixture);
// 0x1047e540 (vt +0xa0) (inferred; the pool's score text is only the assertion's detail): a fixture's winning side
// (1, 2; 0 undecided; -1 for no fixture, with the assertion shown).
std::uint32_t fixture_winner(Cpu&, std::uint32_t cup, std::uint32_t fixture);
// 0x1047bbb0: the fixture of a tie for the screens: the FIX_MAN lookup of round `round`, tie `tie` from today's date,
// moved back day by day (the byte at TOC -0x79ec counts three times) while the match found is already played (+0x43
// = -1). 0 when a argument is negative (assertion) or no fixture is found.
std::uint32_t screen_fixture(Cpu&, std::uint32_t cup, std::uint32_t round, std::uint32_t tie);
// 0x104864d0 (called by FIX_MAN.CPP): looks up the two legs of round `round` (two weeks before the round's
// fixture date, then a day after the first) and stamps their +0x30 with the round's label plus 3 and 4.
void legs_into_fixtures(Cpu&, std::uint32_t cup, std::uint32_t round, std::uint32_t seed, std::uint32_t extra);
// 0x104854f0 / 0x10485570 (call sites: the derived cups' calendars, 900+): a round's draw date and fixture date (a
// calendar day: day of month, month, year, weekday, the year shifted by `shift` for the date's arithmetic and unshifted
// for the stored byte). 0x10485570 also stores the match-day kind (+7) and a word (+0x14) from the caller's stack.
void set_draw_date(Cpu&, std::uint32_t rounds, std::uint32_t round, std::uint32_t day, std::uint32_t month,
                   std::uint32_t year, std::uint32_t weekday, std::uint32_t shift);
void set_fixture_date(Cpu&, std::uint32_t rounds, std::uint32_t round, std::uint32_t day, std::uint32_t month,
                      std::uint32_t year, std::uint32_t weekday, std::uint32_t kind, std::uint32_t shift,
                      std::uint32_t tail);
// 0x10485600: slot `slot` (0-7) of a round: two bytes and a word. Each out-of-range argument shows an assertion and
// exits (and, where exit returns, the slot is written all the same).
void set_round_slot(Cpu&, std::uint32_t rounds, std::uint32_t round, std::uint32_t slot, std::uint32_t first,
                    std::uint32_t second, std::uint32_t third, std::uint32_t tail);
}
