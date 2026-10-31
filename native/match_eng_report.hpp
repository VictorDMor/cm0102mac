#pragma once
// MATCH_ENG.CPP, the review of the match state after each step and the restarts it
// leads to (play state 0x7928 of the match object). Original entries of the report block.
#include "match_eng.hpp"

namespace cm::aot::match_eng {
// 0x101e50c0: play state 2, the restart where the ball lies: every player takes the
// formation cell for the ball's cell, the taker goes to the ball.
void restart_state_2(Cpu&, std::uint32_t match);
// 0x101e53a0: play state 3, a restart from the side line or end line.
void restart_state_3(Cpu&, std::uint32_t match);
// 0x101e59b0: play states 6 and 7, a free kick or throw-in.
void restart_state_6(Cpu&, std::uint32_t match);
// 0x101e6420: play state 4, a restart from the goal area.
void restart_state_4(Cpu&, std::uint32_t match);
// 0x101e66b0: play state 5, a penalty.
void restart_state_5(Cpu&, std::uint32_t match);
// 0x101e68b0: play state 8, the kick-off.
void restart_state_8(Cpu&, std::uint32_t match);
// 0x101e6c50: after each step of the match: tired players, tactic changes, substitutions,
// the lines of commentary, and the restart for the current play state.
void review_play(Cpu&, std::uint32_t match);
}
