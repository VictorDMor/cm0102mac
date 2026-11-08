#pragma once
// MATCH_MAN.CPP squad_b block: the side's tactic rating.
#include "guest_abi.hpp"

namespace cm::aot::match_man {
using guest::Cpu;
// 0x1020c5d0 (called by MATCH_ENG, 0x10206ea0 and 0x1020d7d0): rates the
// tactic of a match side record (`side`: match at +0x14, side number at
// +0x1a, club +0x34, rival club +0x38, fixture +0x3c). It judges the 20
// players of both sides through 0x10219e40 (when the match is live), asks
// 0x101f21d0 for five situation bytes, and multiplies a strength through
// factors of the ratings, results and manager; the strength is stored as a
// single at match + 0x30f0 + side * 4 and the routine returns, in r3, a
// class: 1 to 5, or (when the stored strength is within a tolerance of the
// last one in a live match) the byte at match + 0x1c4 + side. Calls MATCH_ENG
// 0x101eee20 and 0x101f21d0, COMP_STATS 0x10459b50, DATE 0x10718130, the
// libm pow 0x10780fc0 and 0x10775540 (abs).
std::uint32_t classify_tactic(Cpu& c, std::uint32_t side);
}
