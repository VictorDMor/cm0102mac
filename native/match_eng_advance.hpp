#pragma once
// MATCH_ENG.CPP, match advancement: the loop that plays a match forward one tick at a
// time, the pending restart (free kick, throw-in, ...) and the ball's drift while play
// is stopped. Original entries of the advance block.
#include "match_eng.hpp"

namespace cm::aot::match_eng {
// `bl 0x101d3c90`: the match's random number below `bound` (RNG draw), left in r3.
inline std::int32_t draw(Cpu& c, std::uint32_t bound, std::uint32_t back) {
    c.set(3, bound);
    call(c, 0x101d3c90, back);
    return std::int32_t(c.r(3));
}
// 0x101d3f60: plays `count` ticks (stops early when the match ends or a break comes).
// Returns 1 (ticks played), 2 (the match is over), 4 (a period ended), 8 (the deadline
// for the stoppage was reached).
std::uint32_t advance(Cpu&, std::uint32_t match, std::uint32_t count);
// 0x101d5760: whether the flag 0x1000 is set in a side's team record.
std::uint32_t side_flag_1000(Cpu&, std::uint32_t match, std::uint32_t side);
// 0x101d5810: a message to the target object, one player's side in `side`.
void post_event(Cpu&, std::uint32_t target, std::uint32_t id, std::uint32_t source, std::uint32_t side, std::uint32_t last);
// 0x101d5880: whether play continues after a pending restart is looked at (0: it was cleared).
std::uint32_t restart_still_valid(Cpu&, std::uint32_t match);
// 0x101d5980: resolves the pending restart (foul, free kick...): posts its messages, moves the ball
// and the players. Returns 0 when it ended without a result, otherwise 1.
std::uint32_t resolve_restart(Cpu&, std::uint32_t match);
// 0x101d6710: a message about two players.
void post_player_event(Cpu&, std::uint32_t target, std::uint32_t id, std::uint32_t source, std::uint32_t first,
                       std::uint32_t second, std::uint32_t last);
// 0x101d6780: moves every player of the offender's side by a factor, the offender at full weight.
void adjust_side_around(Cpu&, std::uint32_t match, std::uint32_t offender, double factor);
// 0x101d6860: adjusts every player of a side by the same amount.
void adjust_side(Cpu&, std::uint32_t match, std::uint32_t side, double amount);
// 0x101d69c0: 1 when the pending restart is the first team's, otherwise 2 (and the pending state is cleared 1 in 200).
std::uint32_t restart_owner(Cpu&, std::uint32_t match);
// 0x101d6a90: recomputes a side's totals (+0x1e8, +0xaf) from its players, then runs the per-player update.
void side_pressure(Cpu&, std::uint32_t match, std::uint32_t side);
// 0x101d6da0: while play is stopped, the ball drifts across the pitch cells by the default turn.
void drift_ball(Cpu&, std::uint32_t match);
// 0x101d6f90: for each player the team's tactics flag (bits 4 and 0x10), calls the update with the other side's record.
void update_teams_players(Cpu&, std::uint32_t match);
}
