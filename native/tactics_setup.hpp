#pragma once
// TACTICS.CPP: setting up the CTACTICS object (tactics.hpp) when a game
// starts or loads, and keeping it in the saved game's "tactics.dat" entry.
// A new game loads the formation records from the preset files in the
// CM3_TACTICS folder (formation_files.hpp); a loaded game reads them, the
// team records and the formation results from the saved game. Each routine
// leaves what the original leaves (see guest_abi.hpp).
#include "formation_files.hpp"
#include "tactics.hpp"

namespace cm::aot::tactics::setup {
// CTACTICS fields besides tactics::field's.
constexpr std::uint32_t result_count = 0x4;  // int32: formation results, one per formation record
constexpr std::uint32_t shapes = 0x8;        // int32[10]: shape codes of the standard formations
// Old saved games (version below 0x3e021) hold 0x1888-byte formation records
// with 32-bit player instructions and 0x8c-byte team records without
// captains.
constexpr std::int32_t old_save_version = 0x3e021;
constexpr std::uint32_t old_formation_size = 0x1888, old_team_size = 0x8c;
constexpr std::int32_t preset_count = 28, default_count = 17;

// 0x103acf80: the constructor. Waits for the game's CD, drops the human
// managers' squad backups, then loads everything from the saved game's
// archive when there is one (0x103b1d00); otherwise loads the 28 preset and
// 17 named default formations from their files, makes a team record for
// every club and 32 more for the human managers, and the formation results.
// Returns the object.
std::uint32_t construct(guest::Cpu& c, std::uint32_t tactics, std::uint32_t archive);
// 0x103b1d00: reads the formation records, team records, formation results
// and shapes from the archive's "tactics.dat" entry, converting old saved
// games; 1 when everything was read.
std::uint32_t load(guest::Cpu& c, std::uint32_t tactics, std::uint32_t archive);
// 0x103b3290: writes them to the archive's "tactics.dat" entry when a game
// is saved; 1 when everything was written.
std::uint32_t save(guest::Cpu& c, std::uint32_t tactics, std::uint32_t archive);
}
