#pragma once
// PLAYER_STATS.CPP: the object's construction (the real-world history import), destruction, and "player stats.dat".
#include "guest_abi.hpp"

namespace cm::aot::player_stats {
// 0x10301540: builds the player-stats object: asks for the CD until it is
// there, opens the history archive, then loads "player stats.dat" from the
// saved game `archive`, or (a new game, or that load failing) starts empty
// records and imports the real-world career histories. Returns the object.
std::uint32_t create_stats(guest::Cpu& c, std::uint32_t stats, std::uint32_t archive);
// 0x103023e0: frees the history archive and the arrays; deletes the object
// when `flags` (int16) > 0. Returns the object.
std::uint32_t destroy_stats(guest::Cpu& c, std::uint32_t stats, std::uint32_t flags);
// 0x10306c00: loads "player stats.dat" from the save archive (an older
// save's history archive is converted too); 1, or 0 after a reported failure.
std::uint32_t load_stats(guest::Cpu& c, std::uint32_t stats, std::uint32_t archive);
// 0x103078a0: saves "player stats.dat" into the save archive; 1, or 0 after a reported failure.
std::uint32_t save_stats(guest::Cpu& c, std::uint32_t stats, std::uint32_t archive);

namespace detail {
// For player_stats_files*.cpp: a history file object on the temporary
// folder's history file, opened in `mode` and kept in register `reg` as the
// original does (0 when operator new fails); `at` is the operator new call.
std::uint32_t new_history_file(guest::Cpu& c, unsigned reg, std::uint32_t pool, std::uint32_t mode, std::uint32_t at);
}
}
