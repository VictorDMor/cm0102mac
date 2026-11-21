#pragma once
// COMP_UTIL.CPP (tools/comp_util_audit.py): competition utilities shared by the national leagues, the cups and
// the screens: ids and round codes, club and fixture queries, the result texts. comp_util_detail.hpp names the
// records the routines read. Each block of the module declares its routines in its own header section below;
// a routine takes the `Cpu` where the original leaves registers a caller can read (comp_util_routines*.inc
// lines call it with the guest ABI's arguments).
//
// Dependencies still generated, called through the dispatcher (c.call): COMP_STATS.CPP 0x10459b50 (a club's row
// in a competition's statistics), the boundary routine 0x104abf40 (whether the statistics owner marks a
// competition record), and the league's virtual +0xa0 through the pointer glue 0x10781320.
#include "guest_abi.hpp"

namespace cm::aot::comp_util {
using guest::Cpu;

// Block `numeric_ids` (comp_util_ids.cpp): pure functions of a number.
// 0x10464010: the id of the series (the parent competition) a competition id belongs to, from fixed id ranges;
// -1 when it belongs to none. Leaves no register but r3.
std::int32_t parent_series(std::int32_t competition);
// 0x10464880: the last decimal digit of a value below 1000 (the remainder of the signed division by ten, as a
// signed byte), 0 from 1000 on. Leaves r4 as the original does (the sign of the quotient, below 1000).
std::int8_t last_digit(Cpu&, std::int32_t value);
// 0x104648c0: a value below 1000 without its last digit, a halfword; 1000 and above unchanged. Leaves r4 (below
// 1000) and r5 (the value, sign-extended) as the original does.
std::int16_t round_down_to_ten(Cpu&, std::int16_t value);

// Block `club_queries` (comp_util_clubs.cpp, comp_util_race.cpp).
// 0x104646d0: whether competition `competition` is one of the three competitions the words at TOC -0x6888,
// -0x67bc and -0x6734 name, or a league (kind 2) with clubs whose first standing row's club belongs to the
// league's own record (club +0x30 or +0x38). Leaves r4 (the last table row read) and r5 (the standing row, when
// reached) as the original does.
bool competition_is_active(Cpu&, std::int32_t competition);
// 0x10464900: the class (1..7) of a fixture for `club`, from the two clubs' reputations (+0x1ec, in steps of
// 500) and the fixture's goal bytes; which of the two sides `club` is selects the table. Leaves r4..r9 as the
// original does.
std::uint32_t fixture_class(Cpu&, std::uint32_t club, std::uint32_t fixture);
// 0x10464160: whether two clubs of one league (kind 2, without a parent) are contenders in the same race:
// 0, 1 or 3, from the two clubs' COMP_STATS rows (0x10459b50), the difference of their bytes +0x76 against a
// limit from the league's club count, a weighted average of the rows' bytes +0x75..+0x78 per club and the
// league's virtual +0xa0 (inferred: promotion places). Leaves the frame's locals as the original does
// (readers in STAFF_CONTRACTS.CPP, SHORTLIST_MANAGER.CPP, SPAIN_RULES.CPP and a boundary routine read them);
// r4-r10 are not left as the original does (no caller reads them: tools/register_contract.py --after).
std::uint32_t contenders_class(Cpu&, std::uint32_t first_club, std::uint32_t second_club);

// Block `comparators` (comp_util_compare.cpp): the unit's qsort comparators. The sort (QSORT4X.CPP 0x10768300)
// calls each through its transition vector with two pointers to the elements and reads r3 (the three-way
// result) and nothing else; the comparators still leave r4-r7 (and f0, f1) as the original does. A "slot" is
// the address of an array element holding a pointer to the record.
// 0x10463580: records (not slots) with a double at +4, the larger first; equal and unordered compare equal.
// Sets FPSCR as the original's fcmpo does.
std::int32_t compare_score_descending(Cpu&, std::uint32_t first, std::uint32_t second);
// 0x104647a0: slots of clubs, by reputation (+0x1ec) descending. Leaves r4 (the first reputation), r5 (the first club).
std::int32_t compare_club_reputation(Cpu&, std::uint32_t first_slot, std::uint32_t second_slot);
// 0x104647c0: records (not slots), by the signed byte at +4 ascending. Leaves r4 (the second byte, unsigned).
std::int32_t compare_tag_ascending(Cpu&, std::uint32_t first, std::uint32_t second);
// 0x104647e0: slots of clubs, empty slots last, by reputation descending. Leaves r4 (the second club).
std::int32_t compare_club_reputation_nulls_last(Cpu&, std::uint32_t first_slot, std::uint32_t second_slot);
// 0x10464830: slots of clubs, empty slots last, by reputation ASCENDING (the twin of 0x104647e0 with the
// difference the other way round). Leaves r5 (the first club).
std::int32_t compare_club_reputation_ascending(Cpu&, std::uint32_t first_slot, std::uint32_t second_slot);
// 0x10477cd0: slots of clubs, clubs without a division record (+0x34) last, then by that record's reputation
// descending, its id (word +0) descending and the club's byte +0x23c ascending. Leaves r4..r7 as the original.
std::int32_t compare_club_division(Cpu&, std::uint32_t first_slot, std::uint32_t second_slot);
// 0x104777c0: 0x50-byte rows of the COMP_SCREENS list (not slots) by their group byte (+0x42), halfwords +0x32,
// +0x34, +0x30 (each ascending) and the club names (UTILS 0x107465f0, case-sensitive: its r5 is 1). Frames its call.
std::int32_t compare_screen_rows(Cpu&, std::uint32_t first, std::uint32_t second);

// Block `interpolation` (comp_util_interpolation.cpp).
// 0x10476780: `index * (to - from) / (count - 1)` as a signed byte (the mean of the two ends for a count of 1);
// outside 0..100 the game's assertion (line 0x256e) is reported and the value clamped. Leaves r4, r6 and r7
// as the original does (r7 the count; r4 the index and r6 the first end when the count is not 1, r4 the clamp
// after an assertion).
std::int8_t interpolated_share(Cpu&, std::int8_t from, std::int8_t to, std::int16_t index, std::int16_t count);

// Block `rating_sort` (comp_util_rating.cpp).
// 0x10477870: a club's rating for seeding: -10,000,000 for no club; else the finance balance / 300 (the finance
// manager's account, TOC -0x7ab8) or, without a manager, the club's word +4 / 400; plus 16 x its reputation;
// plus the worth of the fifteen places the computer fills from the club's members with player data
// (0x10210c50, each pick rated by 0x10216830).
std::int32_t club_strength(Cpu&, std::uint32_t club);
// 0x10477b40: sorts `count` club pointers by club_strength, best first (a selection sort on a copy of the ratings,
// swapping while the earlier is lower; empty slots rate 0). A failed allocation reports the game's assertion
// (line 0x292b) and leaves the array. Leaves r4-r10 as the original's loops end (a caller's virtual call
// reads r8-r10); no result.
void sort_by_strength(Cpu&, std::uint32_t clubs, std::int32_t count);

// Block `comp_screens_list` (comp_util_screens.cpp).
// 0x10477490: orders the 0x50-byte rows of a COMP_SCREENS list (a NETWORK utility object: count +0, rows from +4,
// cursor +0xc): copies them out, sorts the copy with compare_screen_rows, empties the list and adds the rows back.
// Answers true; false after reporting the game's assertion for no list (line 0x2781), no rows (0x278c) or a failed
// allocation (0x2794), leaving the list as it was.
bool sort_screen_list(Cpu&, std::uint32_t list);
}
