#pragma once
// COMP_UTIL.CPP: the records, globals and shared pieces its routines read (comp_util.hpp has the routines).
// Field meanings come from how this module and its callers use them; "(inferred)" marks a name no reader has
// confirmed. The competition object's own fields are comp_detail.hpp's (`competition::`) and the league's
// league_base_detail.hpp's (`league::`).
#include "comp_detail.hpp"
#include "guest_fpu.hpp"
#include "league_base_detail.hpp"
#include "league_base_news_detail.hpp"
#include "comp_util.hpp"

namespace cm::aot::comp_util {
using guest::Fpu;
using guest::Frame;
using comp::invoke;
using comp::sext16;
using comp::sext8;
using comp::virtual_call;
using league_base::news::Held;
namespace competition = comp::competition;
namespace league = league_base::league;  // the league object's fields; its kind byte (+0x43) is 2

// TOC slots (`lwz rX,slot(r2)`, `c.toc(slot)`). The unit's constants are slot -0x47e0 (a table of doubles,
// below); its string pool is -0x47e4 (not used by these routines).
namespace toc {
inline constexpr std::int32_t
    constants = -0x47e0,       // the unit's doubles, 0x10839d00 (constant:: below)
    stats_owner = -0x7b34,     // cell: the object 0x104abf40 asks about a competition record
    special_a = -0x6888,       // cells: three competition ids that 0x104646d0 accepts at once (inferred)
    special_b = -0x67bc,
    special_c = -0x6734,
    pool = -0x47e4,            // the unit's strings: +0 the file name, +0xe the report format, +0x1b the report title
    message_buffer = -0x7ba8,  // the assertion report's shared message buffer
    success_flag = -0x7bac,    // cell: the byte every assertion report clears
    finance_manager = -0x7ab8, // cell: the finance object 0x10103510 looks accounts up in (0 when there is none)
    screens_vector = -0x6190,  // the transition vector of the screen-row comparator 0x104777c0
    competitions = comp::toc::competitions,   // cell: the competition objects by id
    records = comp::toc::records;             // cell: the 0x6c-byte competition records by id
}

// The unit's doubles (`constants` + offset, read from guest memory: never host literals). +8 is the bias of a
// signed integer converted through the stack (0x4330000080000000), +0x50 the bias of an unsigned one
// (0x4330000000000000); the others are the weights and thresholds of 0x10464160 (their values are the game's).
namespace constant {
inline constexpr std::uint32_t signed_bias = 0x08, limit_scale = 0x10, weight_a = 0x18, weight_b = 0x20,
                               games_scale = 0x28, ramp_start = 0x30, ramp_scale = 0x38, share_divisor = 0x40,
                               promotion_divisor = 0x48, unsigned_bias = 0x50;
}
// The high word of a converted integer's double: the original stores it beside the integer on its frame, loads
// the pair as a double and subtracts the bias.
inline constexpr std::uint32_t conversion_high = 0x43300000;

// The club row (0x248 bytes: comp_detail.hpp `club`).
namespace club {
inline constexpr std::uint32_t competition_record = 0x30,  // word: the club's league record (its +0 the competition id)
                               value = 0x04,               // word: what the ranking divides by 400 without a finance manager (inferred)
                               division_record = 0x34,     // word: a competition record (+0 id, +0x18 reputation) the sorts order by (inferred)
                               second_record = 0x38,       // word: a second competition record (inferred)
                               reputation = 0x1ec,         // halfword
                               name = 0x221,               // the club's name (a C string), what the screens' lists compare last
                               tiebreak = 0x23c;           // signed byte: the last key of the division sort (inferred)
}
// The 0x6c-byte competition record's reputation, which the division sort compares (comp::record::reputation).
// A row of the COMP_SCREENS list (0x50 bytes, copied whole): its club and the keys 0x104777c0 sorts by.
namespace screen_row {
inline constexpr std::uint32_t club = 0x1c, key_30 = 0x30, key_32 = 0x32, key_34 = 0x34 /* halfwords */, group = 0x42 /* signed byte */, size = 0x50;
}
// A record with a double at +4 (0x10463580's sort key).
namespace scored { inline constexpr std::uint32_t score = 0x04; }
// A byte at +4 (0x104647c0's sort key).
namespace tagged { inline constexpr std::uint32_t tag = 0x04; }
// A club's row in a league's statistics (COMP_STATS.CPP 0x10459b50): bytes (inferred: the season's figures).
namespace stats_row {
inline constexpr std::uint32_t games = 0x04,     // byte: games played, below 25 the early-season weights apply
                               figure_75 = 0x75, figure_76 = 0x76, figure_77 = 0x77, figure_78 = 0x78;
}
// A fixture (the game's match row).
namespace fixture {
inline constexpr std::uint32_t first_club = 0x1c, second_club = 0x20,
                               goals_43 = 0x43, goals_44 = 0x44,  // signed bytes: a leg's two scores (inferred)
                               goals_47 = 0x47, goals_48 = 0x48,
                               goals_4b = 0x4b, goals_4c = 0x4c;
}
// What the unit calls of other modules.
namespace callee {
inline constexpr std::uint32_t allocate = 0x10775130, release = 0x10775170, fill_memory = 0x10778300,  // new[], delete[], memset
                               finance_account = 0x10103510, divide64 = 0x10773960, member_ids = 0x100d06f0,
                               choose_places = 0x10210c50, rate_person = 0x10216830;

inline constexpr std::uint32_t stats_row = 0x10459b50,       // COMP_STATS.CPP (statistics object, club) -> row, or 0
                               marks_record = 0x104abf40;    // (statistics owner, competition record) -> byte, nonzero: excluded
}
// The weights a stats row's four figures get: a base weight, scaled while the club has played fewer than 25
// games (0x10464160).
struct Weights {
    double figure_75, figure_76, figure_78, figure_77;
};
}
