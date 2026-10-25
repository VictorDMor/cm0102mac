#pragma once
// PLAYER_STATS.CPP: a person's career table (this season's records and his
// history rows, six rows per season) and the sums the game reads from it.
#include "guest_abi.hpp"

namespace cm::aot::player_stats {
// 0x10305700: appearances a person made for a club (his senior-club rows at it, all seasons).
std::uint32_t club_appearances(guest::Cpu& c, std::uint32_t person, std::uint32_t club);
// 0x103058f0: goals a person scored for a club (his senior-club rows at it, all seasons).
std::uint32_t club_goals(guest::Cpu& c, std::uint32_t person, std::uint32_t club);
// 0x10305ae0: goals a person scored in senior club football, all seasons and clubs.
std::uint32_t career_goals(guest::Cpu& c, std::uint32_t person);
// 0x10305c60: builds a person's career table (CareerRow[], malloc'd; row count at `count`), 0 on failure.
std::uint32_t career_table(guest::Cpu& c, std::uint32_t stats, std::uint32_t person, std::uint32_t count);
// 0x10309cc0: years since a person's last season at a club (0: there now, -1: never or refused).
std::uint32_t seasons_since_club(guest::Cpu& c, std::uint32_t person, std::uint32_t club);
}
