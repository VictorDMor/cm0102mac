#pragma once
// MATCH_MAN.CPP lineup_a block: the computer's lineup choice for a side.
#include "guest_abi.hpp"

namespace cm::aot::match_man {
using guest::Cpu;
// 0x10206ea0 (called only by 0x102065f0, once per side that picks its own lineup):
// the computer's lineup choice for `side` (a match side record: club at +0x34, the
// opposing club at +0x38, 50 squad pointers at +0x2c). `team` is the opposing club
// the caller passes in r4 (kept at the caller's parameter slot sp+0x1c; it may not
// be 0, or the assertion at 0x10206f68 reports and exits). `mode` is the caller's r5
// (0, 1 or 2, taken as a signed byte): 2 also announces the squad to the registry,
// 1 registers the outright picks and turns into 2 when none was made.
// Phases, in order: the swap check (urgency byte and strength of the fixture), the
// first scan of the squad and arrangement, the second scan, the flag word, the pick
// order, the per-slot tables, the second arrangement, the duplicate check, the
// improvement loop through the squad registry, the final picks (each slot handed to
// 0x1020e8f0) and the tactic's flag bits. Returns in r3 the number of slots whose
// player was registered (the byte at sp+0x906), a zero-extended byte.
// Calls MATCH_MAN 0x1020a290, 0x1020c5d0, 0x1020d7d0, 0x1020e8f0, 0x102102d0,
// 0x10210c50 and 0x1021a240 through match_man::call, other modules through the dispatcher.
std::uint32_t computer_lineup(Cpu& c, std::uint32_t side, std::uint32_t team, std::uint32_t mode);
}
