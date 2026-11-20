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
}
