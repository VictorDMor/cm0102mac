#pragma once
// PLAYER_STATS.CPP of the original (0x10301540-0x10309ee0): the player-stats
// object (TOC -0x7b3c, 0x48 bytes): each player's season records by
// competition type, the real-world career histories imported when a game is
// created, the stats screens' texts and values, the ratings the match engine
// and recruitment ask for, the players' relations to clubs and nations, and
// "player stats.dat" / the history files. Each routine leaves what the
// original leaves (guest_abi.hpp); routine bounds are in
// tools/player_stats_audit.py.
#include "player_stats_relations.hpp"
#include "player_stats_queries.hpp"
#include "player_stats_text.hpp"
#include "player_stats_records.hpp"
#include "player_stats_daily.hpp"
#include "player_stats_files.hpp"
