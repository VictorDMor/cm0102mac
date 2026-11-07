#pragma once
// DATABASE.CPP's people block, part "initial": typed functions (see database_people.hpp).
#include "guest_abi.hpp"

namespace cm::aot::database {
using guest::Cpu;

// 16 * (the people count) trials (name inferred: a shuffle of the generated people table, 0x70-byte rows). Each
// trial draws two row indexes below the count from the sample pool (the compiler's inline range_sample: a wide
// count of more than 65535 scales in floating point, calls range_sample and clamps to count - 1). The rows swap
// when they differ, both have a player record (+0x14), both or neither a staff record (+0x1c), both a detail
// record (+0x24) and, unless the detail records share their +0x2c and +0x30 words, person_exempt_from_club_rule
// answers alike for the two with club 0 and person_club_gate (flag 1) lets each person into the club of that
// person's OWN detail record (+0x2c). A swap exchanges the rows except their first word and their detail pointer (+0x24), copying the five
// dates (+0x34..) through 0x10082c10; a pool sample's parity then decides whether the first dates swap again,
// and the halfwords at +0x4 and +0x6 of the two player records swap back. Entry 0x100b2b80.
void shuffle_person_rows(Cpu&);    // 0x100b2b80
}
