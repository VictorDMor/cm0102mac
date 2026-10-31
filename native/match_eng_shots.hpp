#pragma once
// MATCH_ENG.CPP, shots: each side's attacking tendencies (0x101df110) and the routines that pick a
// player of a side by a weighted, randomised score. Original entries of the shots block.
#include "match_eng.hpp"

namespace cm::aot::match_eng {
// 0x101df110: sums the side's players (20 from side*0x2350+0x311c) into the seven halfword
// tendencies at match+0x1f4+side*2.., then scales them by the fixture's rating gap.
void update_side_tendencies(Cpu&, std::uint32_t match, std::uint32_t side);
// 0x101e1640..0x101e22e0: the index of the on-pitch player of `side` with the highest weighted
// random score (the weight comes from the flag halfword of 0x10289560 and the player's record), or -1.
std::uint32_t choose_candidate_weighted(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t reduced);
std::uint32_t choose_candidate_by_mode(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t mode);
// -1 also when the final draw of five is zero.
std::uint32_t choose_candidate_ranked(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t mode);
std::uint32_t choose_candidate_simple(Cpu&, std::uint32_t match, std::uint32_t side);
std::uint32_t choose_candidate_by_traits(Cpu&, std::uint32_t match, std::uint32_t side);
// 0x101e25a0: the low and high nibbles of a player's rating byte in the side's table, stored as bytes.
void role_nibbles(Cpu&, std::uint32_t match, std::uint32_t player, std::uint32_t low_out, std::uint32_t high_out);

namespace shots_detail {
using namespace guest;
// One bit of the flag halfword (extsh, rlwinm, cmpwi 0).
inline bool flagged(Cpu& c, std::uint32_t flags, std::uint32_t mask) {
    return compare(c, std::int32_t(std::int16_t(flags)) & std::int32_t(mask), 0) != 0;
}
// The compiler's int-to-double idiom: the integer with its sign flipped under 0x43300000 on the
// stack, read back as a double and reduced by the pool's 2^52+2^31 (single or double rounding).
inline double from_int_single(Cpu& c, Fpu& fp, std::uint32_t slot, std::uint32_t value, double magic) {
    c.put32(slot, 0x43300000);
    c.put32(slot + 4, value ^ 0x80000000);
    return fp.fsubs(c.f64(slot), magic);
}
inline double from_int(Cpu& c, Fpu& fp, std::uint32_t slot, std::uint32_t value, double magic) {
    c.put32(slot, 0x43300000);
    c.put32(slot + 4, value ^ 0x80000000);
    return fp.fsub(c.f64(slot), magic);
}
// fctiwz then stfd to the stack: the word the original reads back (the low half).
inline std::uint32_t truncated(Cpu& c, Fpu& fp, std::uint32_t slot, double value) {
    c.put64(slot, fp.fctiwz(0, value));
    return c.u32(slot + 4);
}
inline std::uint32_t draw(Cpu& c, std::uint32_t bound, std::uint32_t back) {
    c.set(3, bound);
    call(c, 0x101d3c90, back);
    return c.r(3);
}
}
}
