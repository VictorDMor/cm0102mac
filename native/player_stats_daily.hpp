#pragma once
// PLAYER_STATS.CPP: the daily and match-day updates of the season records and the history files.
#include "guest_abi.hpp"

namespace cm::aot::player_stats {
// 0x103024a0: adds a played match to both sides' players' season records
// (its category and senior club), the rating sums, each player's league
// games and goals for his club and his last five ratings; 0 on a bad argument.
std::uint32_t record_match(guest::Cpu& c, std::uint32_t stats, std::uint32_t fixture, std::uint32_t match);
// 0x10302b20: archives the seasons of every contracted player of the clubs
// whose nation has the given calendar (archive_seasons, no move); 0 on a bad argument.
std::uint32_t archive_nation(guest::Cpu& c, std::uint32_t stats, std::uint32_t calendar);
// 0x10308090: the player's season record of a category, taking a freed
// record or a new one (growing the pool by 100) when he has none; 0 on failure.
std::uint32_t add_season_record(guest::Cpu& c, std::uint32_t stats, std::uint32_t player, std::uint32_t category);
// 0x10308350: writes the person's season records to the history archive and
// clears them, adds an empty senior-club row for a move, and recounts his
// league games and goals for club `club_id`; 0 on a bad argument.
std::uint32_t archive_seasons(guest::Cpu& c, std::uint32_t stats, std::uint32_t person, std::uint32_t other_club,
                              std::uint32_t transfer, std::uint32_t loan, std::uint32_t club_id, std::uint32_t third);
// 0x10308de0: the season record category (1-5) a fixture counts as for a club; 2 on a bad argument.
std::uint32_t competition_type(guest::Cpu& c, std::uint32_t fixture, std::uint32_t club);
// 0x10309040: frees the person's season records and clears his entry and history.
void clear_seasons(guest::Cpu& c, std::uint32_t stats, std::uint32_t person);
}
