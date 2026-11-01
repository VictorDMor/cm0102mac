#pragma once
// MATCH_ENG.CPP, tactics block A: choosing a player's challenge target and
// destination cell, the cell assessments behind it, and the actor ordering.
// Original entries 0x101d70d0-0x101d95a0.
#include "match_eng.hpp"

namespace cm::aot::match_eng {
// 0x101d70d0: the opposing player `player` should challenge: a ball carrier
// beside him, or the best scoring one in the cells around him. Returns the
// chosen player (0 when none).
std::uint32_t choose_challenge_target(Cpu&, std::uint32_t match, std::uint32_t player);
// 0x101d7d30: whether the side's tactical flags have bit 0x400 set.
std::uint32_t side_flag_400(Cpu&, std::uint32_t match, std::uint32_t side);
// 0x101d7dc0: scores the players of the other side in `player`'s cell; adds the
// total to *score_out, copies the two best to `copy_to` (when given); returns a count.
std::uint32_t assess_cell(Cpu&, std::uint32_t match, std::uint32_t player, std::uint32_t score_out,
                          std::uint32_t copy_to);
// 0x101d8120: whether `player` stands off the given row and column and passes the marker tests.
std::uint32_t marker_available(Cpu&, std::uint32_t match, std::uint32_t player, std::uint32_t row,
                               std::uint32_t column);
// 0x101d8260: picks the best cell around `player` to move to (rows and columns in the
// two bytes pointed to); returns whether one was found.
std::uint32_t choose_cell(Cpu&, std::uint32_t match, std::uint32_t player, std::uint32_t row_out,
                          std::uint32_t column_out, std::uint32_t mode);
// 0x101d8d70: whether no live opposing player blocks `player`'s lane.
std::uint32_t lane_clear(Cpu&, std::uint32_t match, std::uint32_t player, std::uint32_t flag);
// 0x101d8f20: sets the side's pressure flag from the weighted count of opponents near its goal.
void rate_goal_pressure(Cpu&, std::uint32_t match, std::uint32_t side);
// 0x101d9080: queues the live players in random order (+0x79f8) and sorts them.
void build_actor_queue(Cpu&, std::uint32_t match);
// 0x101d91b0: the qsort comparator of that queue; `first` and `second` point at queue slots.
std::int32_t compare_actors(Cpu&, std::uint32_t first, std::uint32_t second);
// 0x101d93b0, 0x101d9460: one call per live player of both sides.
void update_players_a(Cpu&, std::uint32_t match);
void update_players_b(Cpu&, std::uint32_t match);
// 0x101d9510: the same call for all 40 players with one extra argument.
void update_all_players(Cpu&, std::uint32_t match, std::uint32_t argument);

// Helpers shared by the block's files.
namespace tactics_a {
using namespace guest;
// A register the routine already holds (its value is kept as it is).
template <unsigned R>
Kept<R> held(Cpu& c) { return Kept<R>(c, c.r(R)); }
// A floating register f14-f31 that the original keeps across calls.
template <unsigned R>
class KeptFloat {
    static_assert(R >= 14 && R <= 31, "KeptFloat: not a non-volatile register");

public:
    explicit KeptFloat(Cpu& c) : c(c) {}
    KeptFloat(Cpu& c, double value) : c(c) { c.set_f(R, value); }
    KeptFloat(const KeptFloat&) = delete;
    KeptFloat& operator=(double value) { c.set_f(R, value); return *this; }
    operator double() const { return c.f(R); }

private:
    Cpu& c;
};
// `lis 0x4330; xoris; stw; stw; lfd`: a signed integer through the stack as a double,
// before the magic constant is subtracted. `at` is the address of the high word.
inline double integer_double(Cpu& c, std::uint32_t at, std::int32_t value) {
    c.put32(at + 4, std::uint32_t(value) ^ 0x80000000u);
    c.put32(at, 0x43300000);
    return c.f64(at);
}
// The same for an unsigned byte (`lbz; stw; lis 0x4330`).
inline double unsigned_double(Cpu& c, std::uint32_t at, std::uint32_t value) {
    c.put32(at + 4, value);
    c.put32(at, 0x43300000);
    return c.f64(at);
}
}
}
