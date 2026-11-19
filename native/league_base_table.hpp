#pragma once
// LEAGUE.CPP, block `table` (league_base_table*.cpp): the standings rows (0x44 bytes each, league+0xb0), their
// order and the match results that feed them. A row holds the club, its position, a pointer to its
// head-to-head entries (0x10 bytes each) and three statistics groups of 0x10 bytes (totals at +0x14, home at
// +0x24, away at +0x34; the league's word at +0xac picks the group the comparators read). Meanings marked
// "(inferred)" come from how the routines use the bytes.
//
// Dependencies on other modules (called through the dispatcher): the allocator 0x10775200 (new, 0x44 bytes
// per row), 0x10775170 delete[], qsort 0x10768300, strcmp 0x1077dc90, the assertion's calls, 0x104646d0
// (a competition record's property), 0x104537b0 (COMP_STATS: the result into the competition's statistics),
// and the other blocks' 0x104a6440 / 0x104a6660 (a copy of the standings and its release).
#include "guest_abi.hpp"

namespace cm::aot::league_base {
// ---- Sorting the standings (league_base_table_order.cpp) ----
// Sorts the rows (`rows`, or the league's own when 0) by the league's tie-break bytes (+0xc7..+0xca), through
// qsort and the comparators below, which read the league from TOC word -0x3750 and a second comparator
// descriptor from -0x3754; writes the rows' position byte (+4) and each row's position history (+0xc). Returns 1.
std::uint32_t sort_standings(Cpu&, std::uint32_t league, std::uint32_t rows);                  // 0x1049eb30
// The head-to-head part of the sort: copies the rows to a new array, adds each tied club's head-to-head
// entries of the other tied clubs into its statistics, sorts that array with compare_tied and writes the
// resulting positions back into the rows' +4 byte (a tied pair shares a position).
void rank_by_head_to_head(Cpu&, std::uint32_t league, std::uint32_t rows);                    // 0x1049f030
// qsort comparators of two row addresses. They consult the league's alternative comparator first (TOC
// -0x3754, a descriptor called through the pointer glue with the same two rows), then the group's points,
// then the tie-break bytes. A null club is equal to everything.
std::int32_t compare_standings(Cpu&, std::uint32_t left, std::uint32_t right);                 // 0x1049f9a0
std::int32_t compare_standings_by_name(Cpu&, std::uint32_t left, std::uint32_t right);         // 0x1049fc80
std::int32_t compare_head_to_head(Cpu&, std::uint32_t left, std::uint32_t right);              // 0x1049fdc0
// The comparator of rank_by_head_to_head's copy rows (points, then the tie-break bytes 1, 2, 3 and 5).
std::int32_t compare_tied(Cpu&, std::uint32_t left, std::uint32_t right);                      // 0x1049ff90

// ---- Match results (league_base_table_result.cpp) ----
// After a match (MATCH_ENG 0x101fcf20): whether `club` would take the title with this match's result (the
// standings are copied, the result applied, sorted, and the others' best possible points compared) (inferred).
std::uint32_t result_decides_title(Cpu&, std::uint32_t league, std::uint32_t club, std::uint32_t fixture);  // 0x104a26b0
// A result into the table (vt +0xc): copies the date, then for an own competition applies the fixture to the
// rows and hands the points to COMP_STATS; for a stage index of the fixture it goes to that stage. `extra` is
// passed on to COMP_STATS. Asserts (lines 0xc7f, 0xc8e, 0xc8f) on a null fixture or a bad stage. Returns 1, 0
// or -1.
std::uint32_t apply_result(Cpu&, std::uint32_t league, std::uint32_t fixture, std::uint32_t extra);  // 0x104a3930
// Applies a fixture to the two clubs' rows: home/away statistics (extra time and penalties by the fixture's
// flag halfword +0x38), totals, the head-to-head entries, and the two clubs' new points through `home_points` /
// `away_points` (halfword pointers, may be 0). `set_decider` records the decider (+0x3d/+0x3e) of the fixture.
void apply_to_rows(Cpu&, std::uint32_t league, std::uint32_t fixture, std::uint32_t rows, std::uint32_t count,
                   std::uint32_t set_decider, std::uint32_t home_points, std::uint32_t away_points);  // 0x104a3b90
// One head-to-head entry: played, won/drawn/lost, points, goals scored and conceded.
void count_head_to_head(Cpu&, std::uint32_t league, std::uint32_t rows, std::uint32_t row, std::uint32_t entry,
                        std::uint32_t scored, std::uint32_t conceded);                         // 0x104a4870

// ---- Fixture slots and queries (league_base_table_queries.cpp) ----
// A fixture's slot byte (vt +0x5c) and the byte written through `marker` (0xff unless the club's chances are
// open); `away` picks the fixture's away club instead of the home club.
std::uint32_t fixture_slot(Cpu&, std::uint32_t league, std::uint32_t fixture, std::uint32_t away, std::uint32_t marker);  // 0x104a9aa0
// Evaluates a row against the league's promotion and relegation places (-1, 0 or a positive number of points;
// writes 0x14 through `marker` when the evaluation is open); no calls.
std::int32_t evaluate_position(Cpu&, std::uint32_t league, std::uint32_t row, std::uint32_t marker);  // 0x104a9bc0
// Clamps the club records' byte +0x23d to `value` (maximum, or minimum when `minimum`) for rows whose status
// byte differs from `excluded` and equals `included` (a status of -3 skips the test).
void limit_club_byte(Cpu&, std::uint32_t league, std::uint32_t value, std::uint32_t excluded, std::uint32_t included,
                     std::uint32_t minimum);                                                   // 0x104aa240
// The clubs of the league's own record: sets their +0x34 and their +0x23c (the row number plus one) (vt +0xac).
void mark_clubs_of_competition(Cpu&, std::uint32_t league);                                    // 0x104aa340
// The points a fixture's result gives `club` (vt +0x98): the league's win or draw points, or 0.
std::uint32_t points_for_result(Cpu&, std::uint32_t league, std::uint32_t club, std::uint32_t fixture);  // 0x104aa430
// The row of `club` among `rows` (the league's own when 0), or 0.
std::uint32_t find_standing(Cpu&, std::uint32_t league, std::uint32_t club, std::uint32_t rows);  // 0x104aa560
// Whether the league has a row of `club` (vt +0x88): 0 or 1.
std::uint32_t has_standing(Cpu&, std::uint32_t league, std::uint32_t club);                    // 0x104aa5c0
}
