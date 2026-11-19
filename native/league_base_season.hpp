#pragma once
// LEAGUE.CPP, block `season` (league_base_season*.cpp): a league's season setup, the fixture list and the schedule
// rounds. Namespace cm::aot::league_base::season.
//
// The schedule is the league's rounds at +0xbc (0x4c bytes each: Round in league_base_detail.hpp): the date, a kind
// byte, eight 8-byte "match slots" from +8 (+0 a word, +4 the three signed bytes the derived leagues' calendars
// set through 0x104a9890) and a word at +0x48. The setup 0x104a00a0 asks the fixtures block (0x104a4970) for the
// order of the standings, lays the round-robin pairings (0x104a10d0, the circle method) over the rounds and files
// one fixture record per match with the fixture manager (0x104aff20/0x104888f0); 0x104a28d0 then picks, for each
// match slot of kind 0..2 of the round a date falls in, the best fixture of the manager's list (a random
// strength, 0x104a3350 the general random draw) and files its news.
//
// Dependencies still generated or owned by other modules (called through the dispatcher / `invoke`): the allocator
// (0x107735f0 new[] with a header, 0x10773610 its release), the fixture order 0x104a4970 (block fixtures), the
// league's virtual +0x84 (COMP.CPP's 0x104530f0 or a derived league's), the fixture manager (0x104aff20 STADIUM,
// 0x104888f0, 0x1048a140, 0x1048a2a0, 0x1048b180, and the list object 0x10738c00/0x10738c20/0x10738e00/0x10738e60),
// news (0x102bdf90, 0x102cb4b0, 0x102cc140), 0x100c7d00, the C library (bsearch 0x10775600, qsort 0x10768300,
// strcmp 0x1077dc90, assertion 0x107755c0, rand 0x1077b770), the club name 0x100d51d0 and the assertion's calls.
// DATE.CPP is called directly (date.hpp). No helper was added to the shared headers: the ones below stay here.
#include "guest_abi.hpp"

namespace cm::aot::league_base::season {
using guest::Cpu;

// 0x104a00a0: the season setup the derived leagues call: 1 on success, 0 after a failed allocation (the assertion).
bool set_up(Cpu&, std::uint32_t league);
// 0x104a10d0: the round-robin pairing matrix (circle method). `table` is an array of `teams + 1` row pointers
// (index 0 unused), each row `teams` words; rows 1..teams get columns 1..teams-1 filled with the opponent of team
// `row` in the round `column`: +n/-n for the fixed team, +k/-k (k the opponent's row, the sign says which side).
// Column 0 of a row is never written. Returns nothing the callers read (the original leaves 1 in r3).
void pair_rounds(Cpu&, std::uint32_t table, std::int32_t teams);
// 0x104a28d0: for each slot of kind 0..2 of the round the date falls in, files the best fixture of the manager's
// list. `date` points at a DATE.CPP date (day, year).
void schedule_fixtures(Cpu&, std::uint32_t league, std::uint32_t date);
// 0x104a3350: the game's general random draw in [0, bound) (toward the bound's sign), by the buffered sample stream.
std::int32_t random_below(Cpu&, std::int32_t bound);
// 0x104a38d0: the order of two rounds (qsort/bsearch comparator): the halfword year +2, then the halfword day +0.
std::int32_t compare_rounds(Cpu&, std::uint32_t left, std::uint32_t right);
// 0x104a9600: the round that follows `previous` in a league whose rounds are drawn (the phase byte at `phase` steps
// 0..3 or draws one random sample). A signed result.
std::int32_t next_round(Cpu&, std::uint32_t previous, std::uint32_t phase, std::uint32_t competition,
                        std::uint32_t clubs, std::uint32_t legs, std::uint32_t rounds, std::uint32_t flags);
// 0x104a9810: the date of schedule round `index` (a day of the month, a month, a year plus `shift`, a weekday: DATE's
// 0x107156f0), its kind byte and the word at +0x48 (the ninth argument, on the stack at the entry sp + 0x38).
void set_round_date(Cpu&, std::uint32_t rounds, std::uint32_t index, std::uint32_t day, std::uint32_t month,
                    std::uint32_t year, std::uint32_t weekday, std::uint32_t kind, std::uint32_t shift, std::uint32_t tail);
// 0x104a9890: the match slot `slot` of schedule round `index`: three signed bytes and a word. Four range
// assertions (lines 0x168c..0x1698) that report and go on writing.
void set_match_slot(Cpu&, std::uint32_t rounds, std::uint32_t index, std::uint32_t slot, std::uint32_t first,
                    std::uint32_t second, std::uint32_t third, std::uint32_t tail);
// 0x104aab30: a preset fixture from a date and two club names: the clubs' ids found among the standings' rows.
void preset_fixture(Cpu&, std::uint32_t fixture, std::uint32_t league, std::uint32_t day, std::uint32_t month,
                    std::uint32_t year, std::uint32_t home_name, std::uint32_t away_name);

namespace detail {
// League fields this block reads (inferred meanings).
namespace field {
inline constexpr std::uint32_t owner_word = 0x24,    // word, -1: none (the second argument of the fixture manager)
                               code = 0x3a, legs = 0x3c, clubs = 0x3e, year = 0x40, owner = 0x44,
                               schedule_count = 0xa6,  // halfword: the number of rounds of the schedule
                               standings = 0xb0, schedule = 0xbc, kind_c6 = 0xc6, flags = 0xdc, half_de = 0xde,
                               byte_e0 = 0xe0, preset_fixtures = 0xe8, record = 0x04, round_half = 0xa8;
}
// TOC words this block loads.
namespace slot {
inline constexpr std::int32_t news = -0x7bf0, fixtures = -0x7a60, fixture_cell = -0x742c, flag = -0x7bac,
                              message = -0x7ba8, pool = -0x470c, doubles = -0x471c, seed = -0x7b90, start = -0x7b8c,
                              end = -0x7b88, cursor = -0x7b84, special_competition = -0x7a30,
                              first_special = -0x7bc0, second_special = -0x6948, order_4714 = -0x4714,
                              by_word = -0x4718;
}
namespace entry {
inline constexpr std::uint32_t rand = 0x1077b770, allocate_array = 0x107735f0, release_array = 0x10773610,
                               bsearch = 0x10775600, qsort = 0x10768300, strcmp = 0x1077dc90,
                               club_name = 0x100d51d0, assert_text = 0x107755c0, fixture_order = 0x104a4970,
                               file_fixture = 0x104aff20, add_fixture = 0x104888f0, remove_fixture = 0x1048a140,
                               find_fixture = 0x1048a2a0, date_blocked = 0x1048b180, list_start = 0x10738c00,
                               list_end = 0x10738c20, list_next = 0x10738e00, list_remove = 0x10738e60,
                               news_start = 0x102bdf90, news_field = 0x102cb4b0, news_file = 0x102cc140,
                               has_fixtures = 0x100c7d00;
}

// The draw the small-bound arm shares: the sample reduced by `bound` (|bound| < 0x10000), negative for a negative bound.
inline std::int32_t reduce(std::uint32_t sample, std::int32_t bound) {
    const auto remainder = bound == -1 ? 0 : std::int32_t(sample) % bound;  // divw of INT_MIN by -1 is undefined on the PowerPC
    return bound > 0 ? remainder : std::int32_t(0u - std::uint32_t(remainder));
}
}
}
