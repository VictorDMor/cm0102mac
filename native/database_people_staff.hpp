#pragma once
// DATABASE.CPP's people block, part "staff": typed functions (see database_people.hpp).
#include "guest_abi.hpp"

namespace cm::aot::database {
using guest::Cpu;

// 0x100dc4d0 (names inferred). Of the up to 50 people in the slots of `club` (+0x98 on, words), the best
// candidate for `person`, or null: a candidate must have a player record, an id below the people count, not be
// the person, have (id + person's id) % 3 != 0, not be one of the person's disliked staff (+0x18 record, +0x28),
// have a contract with the club (0x10082f90), and pass the value tests (0x109a, 0x2ee, the transfer manager's
// answer, statistics). The score adds up statistic tests, the person's favoured staff, bytes of the record and
// the contract, (ids' sum % 500) and the date difference; the highest wins, the first on ties. A person whose
// bytes +0x65/+0x63 or whose days since +0x4c/+0x50 fall outside the id-derived windows answers null.
std::uint32_t best_staff_candidate(Cpu&, std::uint32_t person, std::uint32_t club);
// 0x100dbc20 (names inferred). The same eligibility and score for `candidate` (with no position-mask test);
// true when no eligible person of the club's slots scores more than it, false when one does or the
// candidate is not eligible.
bool no_better_staff_candidate(Cpu&, std::uint32_t person, std::uint32_t club, std::uint32_t candidate);
}
