#pragma once
// MATCH_ENG.CPP, match flow: the end of each period (half time, full time, extra
// time) and the penalty shoot-out that follows. Original entries of the flow block.
#include "match_eng.hpp"

namespace cm::aot::match_eng {
// 0x101d95a0: at the clock's period mark, closes the period and sets up the next
// one. Returns 0 (nothing yet), 1 (half time), 2 (extra time starts), 3 (extra-time half), 4 (over).
std::uint32_t period_ended(Cpu&, std::uint32_t match);
// 0x101da020: reports through the match message routine when a side's line-up
// ratings or a team's strength are out of balance.
void check_lineup_balance(Cpu&, std::uint32_t match, std::uint32_t side);
// 0x101da3c0: fills both sides' shoot-out kick orders (11 slots each).
void choose_shootout_order(Cpu&, std::uint32_t match);
// 0x101da5a0: whether a side's team may still supply a kicker.
std::uint32_t side_can_supply_kicker(Cpu&, std::uint32_t match, std::uint32_t side);
// 0x101da760: the index of `target` among a side's 20 players, or -1.
std::uint32_t player_index(Cpu&, std::uint32_t match, std::uint32_t target, std::uint32_t side);
// 0x101da820: one step of the penalty shoot-out state machine (stage at +0x7929);
// the caller passes mode 4 for stages 1-3 and 3 for stages 4-6: mode 3 resolves the kick directly (the
// stage-4 family, which draws the first kicker), mode 4 plays it out (stage-1 family, finishing at stage 3).
void shootout_step(Cpu&, std::uint32_t match, std::uint32_t mode);
}
