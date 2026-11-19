#pragma once
// CUP.CPP, block `result`: a played fixture applied to the cup (cup_base_result*.cpp). Included at the end of
// cup_base.hpp. Dependencies on routines of other units, all through the dispatcher: COMP_STATS 0x104537b0,
// FINANCE 0x10103510/0x100f71f0, DATE 0x107156b0/0x10717890/0x10715b20, FIX_MAN 0x1048aea0/0x10489900/0x104888f0,
// STADIUM 0x104aff20, COMP_UTIL 0x104648c0, NEWS 0x102cdbf0/0x102bdf90/0x102cb4b0/0x102cc140, the run-time
// library's sprintf and BlockMoveData, and the derived cup's virtuals +0xa0, +0xa4, +0x48, +0x84 and (for a
// stage's cup) +0xc. The CUP.CPP callees 0x1047ba50, 0x10481ad0, 0x10484fd0, 0x10485340 and 0x10485930 belong to
// other blocks and are called through the dispatcher too.
#include "guest_abi.hpp"

namespace cm::aot::cup_base {
// CUP::process_fixture (vt +0x0c): applies the fixture `fixture` of the competition to the cup `cup` (`context` is
// MATCH_DAY's third argument, handed on to the entrant virtual and, for a stage's cup, to the stage). Returns 1
// when a result was applied (3 after a replay was scheduled, 0 or -1 for a fixture that is not this cup's or an
// error).
std::uint32_t process_fixture(Cpu&, std::uint32_t cup, std::uint32_t fixture, std::uint32_t context);  // 0x1047bfb0
}
