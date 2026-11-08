#pragma once
// MATCH_MAN.CPP rating block: how a judge sees a player's ability.
#include "guest_abi.hpp"

namespace cm::aot::match_man {
using guest::Cpu;
// 0x10219e40 (1.4 million calls a day): the ability of `person` (a person
// record whose player data is at +0x14) as `judge` sees it, left in r3 as a
// sign-extended 16-bit value and returned.
// - judge: the person judging (a staff member; may be 0, then the judge is
//   the record at club+0x90, and only for the estimate below);
// - kind: 2 asks for the current ability, anything else the potential (only
//   the low byte counts, as a signed byte; r25 keeps the whole register);
// - known: the judge's estimate already worked out, or negative (-1) to ask
//   0x100cdef0 for it; either way it is divided by 50;
// - club: only read as club+0x90 when `known` is negative and `judge` is 0.
// Calls 0x100cdef0 and 0x100d2f50 (DATABASE.CPP) through the dispatcher.
std::int32_t judged_ability(Cpu& c, std::uint32_t judge, std::uint32_t person, std::uint32_t kind,
                            std::int16_t known, std::uint32_t club);
}
