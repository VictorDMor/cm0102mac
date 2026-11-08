#pragma once
// MATCH_MAN.CPP match block: in-match team management (substitutions and the
// lineup applied to a side).
#include "guest_abi.hpp"

namespace cm::aot::match_man {
using guest::Cpu;
// 0x1020ef70 (called by MATCH_ENG): the side's automatic substitutions. `flag`
// (a byte; non-zero keeps the first slot at position 0) is the second argument.
// Picks the players that stay on, come on and go off, registers the changes with
// the match, rolls the events they cause and applies the lineup (0x102104d0).
void manage_substitutions(Cpu& c, std::uint32_t side, std::uint32_t flag);
// 0x1020d7d0: chooses the lineup record for `side` out of `count` candidate
// persons at `persons` (the arguments `team`, `mode` are the original's r6, r7),
// scoring each position slot, and copies it into the match's per-side area.
// Returns 0 in r3.
std::uint32_t pick_lineup(Cpu& c, std::uint32_t side, std::uint32_t persons, std::uint32_t count,
                          std::uint32_t team, std::uint32_t mode);
// 0x102104d0: applies the position assignment (0x10210c50) to the side's players
// on the pitch and updates their records.
void apply_lineup(Cpu& c, std::uint32_t side);
}
