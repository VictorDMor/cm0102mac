#pragma once
// MATCH_ENG.CPP, contact: how a contact between two players is described and judged, the injuries it causes, the
// queue of contacts of a step and the grid of players around the ball. Original entries of the contact block
// (0x101f5050-0x101f9470).
#include "match_eng.hpp"

namespace cm::aot::match_eng {
namespace contact_detail {
// mulhw by 0x2e8ba2e9, shifted and rounded toward zero: a clock halfword divided by 11 (a minute of the match).
inline std::int32_t divide_by_11(std::int32_t value) {
    auto quotient = std::int32_t(std::int64_t(0x2e8ba2e9) * value >> 32) >> 1;
    quotient += std::int32_t(std::uint32_t(quotient) >> 31);
    return quotient;
}
}
// 0x101f5050..0x101f5230: calls into the discipline object kept inside the match (+0xe030): the match pointer
// becomes that object; the other arguments are passed on (bytes zero-extended, one halfword sign-extended).
void discipline_c260(Cpu&, std::uint32_t match, std::uint32_t a, std::uint32_t b, std::uint32_t c6, std::uint32_t c7,
                     std::uint32_t e, std::uint32_t f, std::uint32_t g);
void discipline_b860(Cpu&, std::uint32_t match, std::uint32_t a, std::uint32_t half, std::uint32_t b, std::uint32_t d);
// The ninth argument of 0x10221e00 is a byte of the caller's parameter area (+0x3b).
void discipline_1e00(Cpu&, std::uint32_t match, std::uint32_t a, std::uint32_t b, std::uint32_t d, std::uint32_t e,
                     std::uint32_t f, std::uint32_t g, std::uint32_t h);
void discipline_bec0(Cpu&, std::uint32_t match, std::uint32_t a, std::uint32_t b, std::uint32_t d, std::uint32_t e,
                     std::uint32_t f, std::uint32_t g);
void discipline_3100(Cpu&, std::uint32_t match, std::uint32_t a, std::uint32_t b, std::uint32_t d, std::uint32_t e);
void discipline_2290(Cpu&, std::uint32_t match, std::uint32_t a, std::uint32_t b, std::uint32_t d, std::uint32_t e);
// 0x101f5280: a flag byte: whether the player holds the ball for his side.
std::uint32_t is_ball_holder(Cpu&, std::uint32_t match, std::uint32_t player);
// 0x101f5350: the contact between two players (either may be 0) with their action codes: scales each player's
// strength by fitness and class, then, by chance, reports the contact to 0x101f6940's callers as one of the
// outcome kinds 1..11.
void describe_contact(Cpu&, std::uint32_t match, std::uint32_t player_a, std::uint32_t action_a, std::uint32_t player_b,
                      std::uint32_t action_b, std::uint32_t flag);
// 0x101f6940: a contact of `severity` of kind `kind` (-1: none) hurts `player`; `culprit` (may be 0) loses morale.
void apply_contact_injury(Cpu&, std::uint32_t match, std::uint32_t kind, std::uint32_t severity, std::uint32_t player,
                          std::uint32_t culprit);
// 0x101f6c60: flagged players leave the pitch with messages; the side's records are updated.
void handle_injured_players(Cpu&, std::uint32_t match);
// 0x101f7100: the squad's best player by his form, written to +0x3100.
void choose_man_of_the_match(Cpu&, std::uint32_t match);
// 0x101f7280, 0x101f7500: players marked in a side's list of performances.
void flag_star_performers(Cpu&, std::uint32_t match, std::uint32_t side);
void flag_players_on_pitch(Cpu&, std::uint32_t match, std::uint32_t side);
// 0x101f75a0: the queued contacts of the step.
void process_contact_events(Cpu&, std::uint32_t match, std::uint32_t queue);
// 0x101f7c10: the grid of the players around the ball, into `grid` (two sides of eleven 8-byte entries from +8).
void fill_contact_grid(Cpu&, std::uint32_t match, std::uint32_t grid);
// 0x101f84e0: the players' contacts with the ball and with each other, step by step.
void update_contacts(Cpu&, std::uint32_t match);
}
