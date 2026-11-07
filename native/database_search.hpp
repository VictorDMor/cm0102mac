// DATABASE.CPP's search block (tools/database_audit.py): typed functions, one per
// original routine, as database.hpp describes.
#pragma once
#include "guest_abi.hpp"

namespace cm::aot::database {
using guest::Cpu;

// 0x100cae30: with a non-zero flag byte, refills the club id array (the first
// word of each club record, one per club) and sorts it with qsort (4-byte items).
void sort_club_ids(Cpu&, std::int8_t refill);
// 0x100d5d70: writes 1..14 into the byte table, stores the low byte of the
// argument at its own global and sorts the table with qsort. Returns the table.
std::uint32_t sort_byte_table(Cpu&, std::uint32_t selector);
// 0x100c4180: compacts the table of 0x1c-byte entries to those whose +4 record is
// null or has a non-zero byte at +0x120 (of the record at its +8 when that is set,
// else of itself), shrinks it with realloc, stores the new count and sorts it.
void compact_ranked_entries(Cpu&);
// 0x100d21f0: dispatcher over 0x100d1d70/0x100d2330 for a person and a slot 1..31;
// a null record or a slot outside 1..31 asserts and answers -1.
std::int32_t search_dispatch(Cpu&, std::uint32_t subject, std::uint32_t record, std::uint32_t slot);
// 0x100d1d70: whether the slot of a record is shown to the person (inferred): 1 when
// either is missing, the lookup flag is clear, the person's lookup finds nothing or
// the answer of 0x106fe280 is 3; with the answers 2 and 1 a pattern of slots (period
// 2 or 4, then 3, shifted by the record's first word modulo 4) hides it, and with any
// other answer it is hidden.
std::uint32_t slot_visible(Cpu&, std::uint32_t person, std::uint32_t record, std::uint32_t slot);

// The squad scans, named for the position they look for (the role byte of the player record: attacker 0x15,
// defender 0x11, midfielders 0x12-0x14, goalkeeper 0xf; inferred from database_detail.hpp) and the field they
// rank by (offsets of the player record, meaning not established) (the club's 50 persons at +0x98; all inferred from the tests):
// each derives a threshold from the club's rating (16-bit at +0x1ec), then counts
// the persons whose player record (+0x14) fits a position test, whose status query
// (0x10187680, global at TOC -0x7ba0) is not 2, whose id is below the global at TOC
// -0x7be4 and whose player rating (16-bit at +4) exceeds the threshold, plus the
// attribute tests listed in database_search_squad.cpp, and answer whether the count
// reaches a limit (a signed-byte count; 0 for no club).
std::int32_t squad_has_more_than_two_midfielders(Cpu&, std::uint32_t club);
std::int32_t squad_has_more_than_four_with_field_44(Cpu&, std::uint32_t club);
std::int32_t squad_has_at_most_one_attacker_by_field_2e(Cpu&, std::uint32_t club);
std::int32_t squad_has_at_most_one_defender_by_field_2e(Cpu&, std::uint32_t club);
std::int32_t squad_has_at_most_one_defender_by_field_36(Cpu&, std::uint32_t club);
std::int32_t squad_has_attacker_by_field_36(Cpu&, std::uint32_t club);
std::int32_t squad_has_more_than_one_midfielder_by_fields_44_3e(Cpu&, std::uint32_t club);
std::int32_t squad_has_more_than_one_midfielder_by_fields_43_37(Cpu&, std::uint32_t club);
// 0x100dd100: whether some person of the club has a natural role (player byte +0xf is
// 20), a status other than 2 and an id below the global; answers 1 or 0.
std::int32_t squad_has_goalkeeper(Cpu&, std::uint32_t club);
std::int32_t squad_has_more_than_two_defenders_with_team_check(Cpu&, std::uint32_t club);
std::int32_t squad_has_more_than_one_attacker_with_team_check(Cpu&, std::uint32_t club);
// 0x100ddb70: a level 0..6 for club `a` (3 without a club) from the ratings of its best
// persons (or the club's own rating when none qualifies), adjusted by the rating gap
// to club `b` and two flags of their table entries.
std::int32_t club_level(Cpu&, std::uint32_t a, std::uint32_t b);
// 0x100d3f30: for every club of the table whose club_of_code (0x100ce320) answers a club and
// sets its found byte: counts the club's people with an id below the global, sorts the linked
// club's 50 persons (qsort, comparator at TOC -0x7980) and removes (0x100c8930) those over
// 50 and, for the lists of 5, 7 and 3 (+0x160, +0x174, +0x190) every person, adding them to
// this club (0x100c82a0) while the list has room; then the single slots +0x90 and +0x94.
void relink_club_squads(Cpu&);
// 0x100c8930: takes `person` out of the slot of `club` that the person's kind byte names (+0x60, or +0x62
// below the range limit; the same kinds add_to_record fills): clears the id word or list slot, false when
// the person is not there. With a date its fields are copied into the person; with flag_a the person's
// club field (+0x20, or +0x24 below the limit) is cleared. A null club answers false; a null person asserts.
bool remove_from_record(Cpu&, std::uint32_t club, std::uint32_t person, std::uint32_t date, std::uint32_t flag_a,
                        std::uint32_t flag_b);

// 0x100d71a0: a bit mask of traits of a person (null person or zero `enabled`: 0), from role tests of the
// player record, tests of its attributes against `noise % m + k` (the noise table walk, one generator draw
// when its pointer passes the bound), the transfer manager's answer (0x103f43b0) and player-statistics
// answers (0x10303560); bits are listed in database_search_traits.cpp.
std::uint32_t person_trait_mask(Cpu&, std::uint32_t person, std::uint32_t enabled);
// 0x100d8260: the second trait mask, of the same machinery, with tactics callees (0x103b05c0, 0x103af570,
// 0x10111240), the id rating (0x10187650) and 0x10216830; `argument` is passed to the tactics callees.
std::uint32_t person_trait_mask_with_team_fit(Cpu&, std::uint32_t person, std::uint32_t argument);
}
