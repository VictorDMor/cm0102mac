#pragma once
// MATCH_MAN.CPP rating_c block: a person rated for one place of a side.
#include "guest_abi.hpp"

namespace cm::aot::match_man {
using guest::Cpu;
// 0x10211e70 (about 70,000 calls a day in a career): the worth of `person` for one place of a side, left in r3
// and returned (-10000 for a missing person or player data, -1000000 for a person withdrawn from the side).
// Arguments in r3-r10 and seven stack words (at sp+0x38 on entry):
// - person, data: the person record and its player data (+0x14);
// - record: the person's record on the side (a match player, +0x1a4 the match), or 0;
// - role: the place's role mask (r6, a halfword), stored at 0x186 of the frame;
// - flags: the components to include (r7, tested bit by bit);
// - club, side, judge: the club (r8), the side record (r9; +0x20 its judge, +0x24 that judge's staff data) and the
//   judge given (r10);
// - the stack: the floor (-1 for none; 0 when a human manager is given), the place's two mask words, its role
//   byte, an extra word, its rating halfword and the human manager's slot list.
// Calls 0x10219e40 (judged_ability) through match_man::call and other modules through the dispatcher.
std::int32_t rating_c(Cpu& c, std::uint32_t person, std::uint32_t data, std::uint32_t record, std::uint32_t role,
                      std::uint32_t flags, std::uint32_t club, std::uint32_t side, std::uint32_t judge);
}
