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
    rng_salt = -0x7b90,        // cell: the pool's salt word (the last refill's value)
    rng_start = -0x7b8c,       // the pool's first sample (the slot holds the address itself, not a cell)
    rng_end = -0x7b88,         // cell: the address of the pool's last sample
    rng_cursor = -0x7b84,      // cell: the address of the sample the next draw reads
    competitions = comp::toc::competitions,   // cell: the competition objects by id
    records = comp::toc::records,             // cell: the 0x6c-byte competition records by id
    clubs = comp::toc::clubs;                 // cell: the 0x248-byte club rows
}

// The unit's doubles (`constants` + offset, read from guest memory: never host literals). +8 is the bias of a
// signed integer converted through the stack (0x4330000080000000), +0x50 the bias of an unsigned one
// (0x4330000000000000); the others are the weights and thresholds of 0x10464160 (their values are the game's).
namespace constant {
inline constexpr std::uint32_t word_range = 0x00 /* the pooled draw's scale: 65535.0 (inferred) */, signed_bias = 0x08, limit_scale = 0x10, weight_a = 0x18, weight_b = 0x20,
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
// What the result headline (0x10464d00, comp_util_headline.cpp) reads of a fixture row beside the above.
inline constexpr std::uint32_t competition_id = 0x00,                       // word: compared with two special competitions' ids
                               stats_leg = 0x08, first_club_id = 0x0c, second_club_id = 0x10,  // words: 0x1029eb20 looks the row up by them
                               record = 0x14,                               // word: the competition's record (comp::record)
                               first_leg = 0x18,                            // word: a record with a name (+0x18) and a gender (+0x4b), or 0
                               date_a = 0x28, date_b = 0x2a,                // halfwords: the arguments of DATE 0x107156b0
                               round_code_30 = 0x30, round_code_32 = 0x32,  // halfwords (the same codes as comp_util.hpp's round texts)
                               tiebreak_first = 0x3d, tiebreak_second = 0x3e;  // bytes (inferred: each side's tie-break value, signed)
}
// The fixture reference 0x10463be0 writes into a news item and 0x10463dc0 reads back (the game's match row, 0x50
// bytes): eighteen of its fields travel as consecutive words of the item, the others are derived. The ids at +0,
// +4, +0xc and +0x10 are -1 for none; each has a pointer (+0x14, +0x18, +0x1c, +0x20) that the reader rebuilds from
// the id (0 for -1). `row_table` is the cell of the 0x50-byte row table the +4 id indexes (inferred: the game's
// fixture rows).
namespace fixture_reference {
inline constexpr std::uint32_t competition_id = 0x00, row_id = 0x04, word_08 = 0x08, first_club_id = 0x0c, second_club_id = 0x10,
                               competition_record = 0x14, row = 0x18, first_club = 0x1c, second_club = 0x20,
                               half_28 = 0x28, half_2a = 0x2a, half_30 = 0x30, half_32 = 0x32,  // halfwords (inferred: the fixture's date and time)
                               byte_3d = 0x3d, byte_3e = 0x3e, byte_42 = 0x42, byte_43 = 0x43, byte_44 = 0x44,
                               byte_47 = 0x47, byte_48 = 0x48, byte_4b = 0x4b, byte_4c = 0x4c;  // signed bytes
inline constexpr std::uint32_t fields = 18;  // words in a news item, from the index the cursor byte names
inline constexpr std::int32_t row_table = -0x7548;  // TOC cell: the 0x50-byte rows the +4 id indexes
inline constexpr std::uint32_t row_size = 0x50;
}
// The history log (the unit's third table): 0x1c-byte rows, one per competition and year. `rows` and `count` are
// TOC cells (the table's pointer and its length); `compare` the DATABASE comparator's transition vector.
// A row's +0 is not written by the appender (kept from whatever the slot held).
namespace history_log {
inline constexpr std::int32_t rows = -0x7a34, count = -0x7a38, compare = -0x7a3c, year_date = -0x7b80,
                              rows_vector = -0x4980;  // the transition vector of the row comparator 0x10477270
inline constexpr std::uint32_t row_size = 0x1c, competition = 0x04 /* the competition's record */, winner = 0x08,
                               runner_up = 0x0c, third = 0x10, host = 0x14, year = 0x18 /* halfword */,
                               grow_step = 0x14 /* rows added when the table is full */, collect_step = 10;
// The comparator's context: two club rows, the words at TOC offsets -0x3768 and -0x376c (written by 0x104770c0).
inline constexpr std::int32_t context_first = -0x3768, context_second = -0x376c;
}
// What the unit calls of other modules.
namespace callee {
inline constexpr std::uint32_t allocate = 0x10775130, release = 0x10775170, fill_memory = 0x10778300,  // new[], delete[], memset
                               finance_account = 0x10103510, divide64 = 0x10773960, member_ids = 0x100d06f0,
                               choose_places = 0x10210c50, rate_person = 0x10216830,
                               set_item_word = 0x102cb4b0, item_word = 0x102cb570,  // NEWS.CPP (item, index[, value])
                               grow_log = 0x100a4ce0, compare_log_rows = 0x100cde70,  // DATABASE.CPP: grow the history log by n rows; its row comparator
                               sort = 0x10768300, search = 0x10775600, resize = 0x107751b0,  // qsort, bsearch, realloc
                               reseed = 0x1077b770;  // the pooled generator's refill

inline constexpr std::uint32_t stats_row = 0x10459b50,       // COMP_STATS.CPP (statistics object, club) -> row, or 0
                               marks_record = 0x104abf40;    // (statistics owner, competition record) -> byte, nonzero: excluded
}
// The weights a stats row's four figures get: a base weight, scaled while the club has played fewer than 25
// games (0x10464160).
struct Weights {
    double figure_75, figure_76, figure_78, figure_77;
};
}
