#pragma once
// MATCH_ENG.CPP, rating predicates: the stage lookup of a fixture pair, the grade posted for
// a fixture, the average of a side's players off the pitch and the big situation predicate.
// Original entries 0x101fd6b0..0x101fdb20, 0x101fdb70 and 0x101fdc10.
#include "match_eng.hpp"

namespace cm::aot::match_eng {
// 0x101fd6b0: how many stages of the competition pair `pair` (a record of three pairs of
// objects at +0x44, +0x4c, +0x54) are needed before `record`'s object is found by 0x100cd980
// (mode 0), or whether the first pair is filled (mode != 0). Returns 0 after an assertion
// (null argument), 1 (stage missing or empty), 2 (stage 1 or 2 is empty), otherwise the stage count.
std::uint32_t stage_number(Cpu&, std::uint32_t pair, std::uint32_t record, std::uint32_t mode);
// 0x101fd900: posts the fixture's grade (1..10, from its year, two byte sums and two random draws).
void post_grade(Cpu&, std::uint32_t match);
// 0x101fdb70: the average of the +0x36 halfword of the side's players whose position is negative.
std::uint32_t bench_average(Cpu&, std::uint32_t match, std::uint32_t side);
// 0x101fdc10: whether situation `kind` (0x21a3..0x21af, others pass when in 0x1f40..0x21e4) holds
// for the side; the cases that draw a rules record store its value through `out` when it is not null.
std::uint32_t situation_met(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t kind, std::uint32_t out);
}
