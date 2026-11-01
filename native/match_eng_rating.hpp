#pragma once
// MATCH_ENG.CPP, rating: how a person's three ratings are worked out for a fixture (0x101f9470 writes
// them into the output record, or into the person's sheet when there is none; 0x101fb580 is the variant
// that looks the person up from a record), and the side tendency stored for a match (0x101fc670).
// Original entries of the rating block.
#include "match_eng.hpp"

namespace cm::aot::match_eng {
// 0x101f9470: the three ratings (rating_a, rating_b, rating_c) of `person` at `club` against `opponent`
// (a team record; +0x1ec is its strength) in `fixture`, scaled by `weight`. `output` receives them as
// halfwords at +0x48, +0x4a, +0x4c; with no output they replace the person's sheet halfwords +0xa, +0x8, +0xc.
void compute_ratings(Cpu&, std::uint32_t output, std::uint32_t person, std::uint32_t club, std::uint32_t opponent,
                     std::uint32_t fixture, double weight, std::uint32_t flag_a, std::uint32_t flag_b);
// 0x101fb580: the same three values from a record whose person is `record+0x20`->, written to the record's first
// three halfwords. `opponent` is the team record whose strength (+0x1ec) is read.
void compute_record_ratings(Cpu&, std::uint32_t record, std::uint32_t club, std::uint32_t opponent, std::uint32_t fixture,
                            double weight, std::uint32_t flag_a, std::uint32_t flag_b);
// 0x101fc670: a side's tendency halfword at match+0x1f0+side*2 from the fixture's team strength.
void compute_side_tendency(Cpu&, std::uint32_t match, std::uint32_t side, double weight, std::uint32_t rate_flag,
                           std::uint32_t flag);

namespace rating_detail {
using namespace guest;
inline std::uint32_t x8(std::uint32_t v) { return u(std::int32_t(std::int8_t(v))); }    // extsb
inline std::uint32_t x16(std::uint32_t v) { return u(std::int32_t(std::int16_t(v))); }  // extsh
inline std::uint32_t add(std::uint32_t a, std::uint32_t b) { return a + b; }
// mulhw by 0x10624dd3 then the shift and the round-toward-zero correction: x / 1000 (shift 6) or x / 500 (shift 5).
inline std::int32_t divide_by_magic(std::int32_t value, unsigned shift) {
    const auto high = std::int32_t((std::int64_t(0x10624dd3) * value) >> 32);
    const auto q = high >> shift;
    return std::int32_t(std::uint32_t(q) + (std::uint32_t(q) >> 31));
}
// The compiler's int-to-double idiom: the integer with its sign flipped under 0x43300000 on the stack, read back.
inline double widen(Cpu& c, std::uint32_t slot, std::uint32_t value) {
    c.put32(slot + 4, value ^ 0x80000000);
    c.put32(slot, 0x43300000);
    return c.f64(slot);
}
// fctiwz then stfd to the stack: the low word the original reads back.
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
