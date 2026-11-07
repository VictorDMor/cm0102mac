#pragma once
// DATABASE.CPP's people block, part "extra": typed functions (see database_people.hpp).
#include "guest_abi.hpp"

namespace cm::aot::database {
using guest::Cpu;

// Generates the non-playing staff of the clubs that lack some (name inferred). First every generated person of
// role 8 must be listed in its club's five-slot list (+0x160), or an assert fires and the pass stops. Then, per
// club (a club of the player's nation with a rating of 500 or LESS, or one that 0x100c7d00 and 0x100ce320
// reject, is skipped), role 8 (five slots at +0x160), role 9 (seven at +0x174) and role 10 (three at +0x190) staff are
// created until the list holds as many as (rating + 500) * slots / 10000 (at least one, at most the slots); each
// new person takes names, a category and a record from 0x100c4400 / 0x100c4cf0 / 0x100c53e0 and values from the
// sample pool (0x107158a0 sets the +0x34 field). Finally a role 5 person for each club without one (id word +0x90)
// that passes the rating gates (500 or less is skipped for a club of the player's nation; 7500 is the gate
// of a club without a linked record, second link or flag byte). Writes "generated %ld extra non playing staff"; answers 1, or 0 after
// an assert (21000 = 0x5208 people generated).
std::uint32_t generate_extra_staff(Cpu&);  // 0x100a3db0
}
