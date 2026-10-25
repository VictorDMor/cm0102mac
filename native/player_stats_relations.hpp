#pragma once
// PLAYER_STATS.CPP: the players' relations to affinity groups (the stats
// object's +0x3c array: person, group, strength 0-10), the history rows'
// merge, and the sort comparators.
#include "guest_abi.hpp"

namespace cm::aot::player_stats {
// 0x103092f0: a person's affinity (0-10) for `club` and `group`: his
// contract's when he has one at a club of his own club's nation (asked only
// when `ask_contract`), 10 when the club's nation's group or `group` is one
// of his nations' groups, else the relation kept for him and `group`.
std::uint32_t affinity(guest::Cpu& c, std::uint32_t stats, std::uint32_t person, std::uint32_t club,
                       std::uint32_t group, std::uint32_t ask_contract);
// 0x103094b0: how strongly a person's nations belong to `group` (0, 100,
// 300, 500 or 1000); used to seed the relations of a new game.
std::uint32_t group_score(guest::Cpu& c, std::uint32_t person, std::uint32_t group);
// 0x10309680: appends a relation (strength at most 10), re-sorting the
// array when `sort`; allocates and grows it by 100 entries.
void add_relation(guest::Cpu& c, std::uint32_t stats, std::uint32_t person, std::uint32_t group,
                  std::uint32_t strength, std::uint32_t sort);
// 0x10309850: removes a person's relation to `group`, if kept.
void remove_relation(guest::Cpu& c, std::uint32_t stats, std::uint32_t person, std::uint32_t group);
// 0x10309940: adds `amount` to a relation (at most 10), or appends it.
void add_to_relation(guest::Cpu& c, std::uint32_t stats, std::uint32_t person, std::uint32_t group,
                     std::uint32_t amount, std::uint32_t sort);
// 0x10309b60: in the history rows (sorted by player and year), orders a
// season's two clubs so that the club of the next season comes second.
void merge_history_rows(guest::Cpu& c);
// 0x10309a00: qsort/bsearch comparator of relation entries (person, then the
// group unless either is -1, a wildcard).
std::uint32_t compare_relations(guest::Cpu& c, std::uint32_t a, std::uint32_t b);
// 0x10309a70: qsort comparator of history rows: by club, then by player.
std::uint32_t compare_history_by_club(guest::Cpu& c, std::uint32_t a, std::uint32_t b);
// 0x10309ae0: qsort comparator of history rows: by player, year, then the first word.
std::uint32_t compare_history_by_player(guest::Cpu& c, std::uint32_t a, std::uint32_t b);
}
