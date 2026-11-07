#pragma once
// DATABASE.CPP's people block, part "swaps": typed functions (see database_people.hpp).
#include "guest_abi.hpp"

namespace cm::aot::database {
using guest::Cpu;

// 100 trials per club: two squad slots (of 50 people each, club +0x98) are drawn from the sample pool, and when
// both people have a player record and their ratings are close (+0x4 within 10 when either has its +0x5e byte
// set, and the byte at +0x5d of the person within 4) the pool's parity decides, three times, whether the players
// swap a pair of bytes (+0x36/+0x1b, +0x3c, +0x2e/+0x2b); the third of these is skipped when the first player's
// byte +0x11 is 20 (the original tests it twice, at the same address). The last swap, of the halfwords at +0x4
// and +0x6, happens whenever the pair qualifies: both outcomes of the +0x11 test reach it (0x100b3b4c).
// Afterwards each person with a player record is passed to 0x100c2cb0 and 0x100c2800 (settle_player_attributes_weighted
// and settle_player_attributes, called directly).
// Name inferred.
void swap_player_attributes_within_clubs(Cpu&);    // 0x100b3710
// Settles two counters of each person (+0x5e, then +0x5f; inferred: remaining days of a status and its
// level) from the age of a date (date_difference against a TOC date), the player's ratings (+0x4, +0x8,
// +0x14, +0x15) and draws of the sample pool, and clears +0x4 and +0x8 of the person. Name inferred.
void settle_person_counters(Cpu&);                 // 0x100b3c70
}
