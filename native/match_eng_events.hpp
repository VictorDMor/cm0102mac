#pragma once
// MATCH_ENG.CPP, events: the minute-by-minute simulation of a match played without the pitch
// view (0x101db340) and the six routines that adjust every player of a side by his role flags.
#include "match_eng.hpp"

namespace cm::aot::match_eng {
// 0x101db340: plays a whole match as a series of minutes: each minute weighs the sides' chances,
// books shots, goals, cards and injuries on the players and the side statistics, then moves
// the clock (half time at 45, full time at 90, extra time to 120 and the penalty shoot-out).
// Writes the final scores into the fixture.
void simulate_match_events(Cpu&, std::uint32_t match);
// 0x101de0a0, 0x101de340, 0x101de610, 0x101de8b0, 0x101deb50, 0x101dee30: add `amount` scaled
// by a per-role factor to the +0x36 halfword of each of a side's 20 players that is on the
// pitch (the factor is chosen by the first set bit of the player's role flags, 0x10289560).
void adjust_players_a(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t amount);
void adjust_players_b(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t amount);
void adjust_players_c(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t amount);
void adjust_players_d(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t amount);
void adjust_players_e(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t amount);
void adjust_players_f(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t amount);

namespace events_detail {
// A local the original keeps in non-volatile register R without setting it where it is
// declared (Kept sets at construction).
template <unsigned R>
class Held {
public:
    explicit Held(Cpu& cpu) : c(cpu) {}
    Held(const Held&) = delete;
    Held& operator=(std::uint32_t value) {
        c.set(R, value);
        return *this;
    }
    operator std::uint32_t() const { return c.r(R); }

private:
    Cpu& c;
};
// x / divisor the way the compiler divided: high word of x*magic (plus x when the magic is
// negative as a signed word), arithmetic shift, plus the sign bit.
inline std::int32_t magic_quotient(std::int32_t x, std::int32_t magic, unsigned shift) {
    auto high = std::int32_t((std::int64_t(magic) * x) >> 32);
    if (magic < 0) high = std::int32_t(std::uint32_t(high) + std::uint32_t(x));
    high >>= shift;
    return std::int32_t(std::uint32_t(high) + (std::uint32_t(high) >> 31));
}
}
}
