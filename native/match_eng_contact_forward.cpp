// MATCH_ENG.CPP, contact: the thin forwards to the discipline object kept inside the match (+0xe030),
// the possession test and the injury a contact causes.
#include "match_eng_contact.hpp"
#include "match_eng_shots.hpp"

namespace cm::aot::match_eng {
using namespace guest;
using namespace shots_detail;

namespace {
// `addis r3,r3,1; subi r3,r3,0x1fd0`: the discipline object sits inside the match.
constexpr std::uint32_t discipline_object_at = 0x10000 - 0x1fd0;
}

// 0x101f5050
void discipline_c260(Cpu& c, std::uint32_t match, std::uint32_t a, std::uint32_t b, std::uint32_t c6, std::uint32_t c7,
                     std::uint32_t e, std::uint32_t f, std::uint32_t g) {
    Frame frame(c, 0x101f5050, 0x40, {});
    const auto at = frame.caller();
    c.put32(at + 0x18, match); c.put32(at + 0x1c, a); c.put32(at + 0x20, b);
    c.put8(at + 0x27, c6); c.put8(at + 0x2b, c7);
    c.put32(at + 0x2c, e); c.put32(at + 0x30, f); c.put32(at + 0x34, g);
    c.set({{3, c.u32(at + 0x18) + discipline_object_at}, {4, c.u32(at + 0x1c)}, {5, c.u32(at + 0x20)}, {6, c.u8(at + 0x27)},
           {7, c.u8(at + 0x2b)}, {8, c.u32(at + 0x2c)}, {9, c.u32(at + 0x30)}, {10, c.u32(at + 0x34)}});
    c.call(0x1021c260, 0x101f50a8);
}

// 0x101f50c0
void discipline_b860(Cpu& c, std::uint32_t match, std::uint32_t a, std::uint32_t half, std::uint32_t b, std::uint32_t d) {
    Frame frame(c, 0x101f50c0, 0x40, {});
    const auto at = frame.caller();
    c.put32(at + 0x18, match); c.put32(at + 0x1c, a); c.put16(at + 0x22, half); c.put32(at + 0x24, b); c.put32(at + 0x28, d);
    c.set({{3, c.u32(at + 0x18) + discipline_object_at}, {4, c.u32(at + 0x1c)}, {5, extended(c.i16(at + 0x22))},
           {6, c.u32(at + 0x24)}, {7, c.u32(at + 0x28)}});
    c.call(0x1021b860, 0x101f5100);
}

// 0x101f5110: the ninth argument is a byte the caller left in its parameter area.
void discipline_1e00(Cpu& c, std::uint32_t match, std::uint32_t a, std::uint32_t b, std::uint32_t d, std::uint32_t e,
                     std::uint32_t f, std::uint32_t g, std::uint32_t h) {
    Frame frame(c, 0x101f5110, 0x40, {});
    const auto at = frame.caller();
    c.put32(at + 0x18, match); c.put32(at + 0x1c, a); c.put32(at + 0x20, b); c.put32(at + 0x24, d);
    c.put8(at + 0x2b, e); c.put8(at + 0x2f, f); c.put8(at + 0x33, g); c.put32(at + 0x34, h);
    c.set(0, c.u8(at + 0x3b));
    c.put32(frame.at(0x38), c.r(0));
    c.set({{3, c.u32(at + 0x18) + discipline_object_at}, {4, c.u32(at + 0x1c)}, {5, c.u32(at + 0x20)}, {6, c.u32(at + 0x24)},
           {7, c.u8(at + 0x2b)}, {8, c.u8(at + 0x2f)}, {9, c.u8(at + 0x33)}, {10, c.u32(at + 0x34)}});
    c.call(0x10221e00, 0x101f5170);
}

// 0x101f5180
void discipline_bec0(Cpu& c, std::uint32_t match, std::uint32_t a, std::uint32_t b, std::uint32_t d, std::uint32_t e,
                     std::uint32_t f, std::uint32_t g) {
    Frame frame(c, 0x101f5180, 0x40, {});
    const auto at = frame.caller();
    c.put32(at + 0x18, match); c.put32(at + 0x1c, a); c.put32(at + 0x20, b);
    c.put8(at + 0x27, d); c.put8(at + 0x2b, e); c.put8(at + 0x2f, f); c.put32(at + 0x30, g);
    c.set({{3, c.u32(at + 0x18) + discipline_object_at}, {4, c.u32(at + 0x1c)}, {5, c.u32(at + 0x20)}, {6, c.u8(at + 0x27)},
           {7, c.u8(at + 0x2b)}, {8, c.u8(at + 0x2f)}, {9, c.u32(at + 0x30)}});
    c.call(0x1021bec0, 0x101f51d0);
}

// 0x101f51e0
void discipline_3100(Cpu& c, std::uint32_t match, std::uint32_t a, std::uint32_t b, std::uint32_t d, std::uint32_t e) {
    Frame frame(c, 0x101f51e0, 0x40, {});
    const auto at = frame.caller();
    c.put32(at + 0x18, match); c.put32(at + 0x1c, a); c.put32(at + 0x20, b); c.put8(at + 0x27, d); c.put8(at + 0x2b, e);
    c.set({{3, c.u32(at + 0x18) + discipline_object_at}, {4, c.u32(at + 0x1c)}, {5, c.u32(at + 0x20)}, {6, c.u8(at + 0x27)},
           {7, c.u8(at + 0x2b)}});
    c.call(0x10223100, 0x101f5220);
}

// 0x101f5230
void discipline_2290(Cpu& c, std::uint32_t match, std::uint32_t a, std::uint32_t b, std::uint32_t d, std::uint32_t e) {
    Frame frame(c, 0x101f5230, 0x40, {});
    const auto at = frame.caller();
    c.put32(at + 0x18, match); c.put8(at + 0x1f, a); c.put8(at + 0x23, b); c.put8(at + 0x27, d); c.put32(at + 0x28, e);
    c.set({{3, c.u32(at + 0x18) + discipline_object_at}, {4, c.u8(at + 0x1f)}, {5, c.u8(at + 0x23)}, {6, c.u8(at + 0x27)},
           {7, c.u32(at + 0x28)}});
    c.call(0x10222290, 0x101f5270);
}

// 0x101f5280: whether the player holds (or last touched) the ball for his side, as a flag byte.
std::uint32_t is_ball_holder(Cpu& c, std::uint32_t match, std::uint32_t player) {
    Frame frame(c, 0x101f5280, 0x40, {.first_gpr = 30});
    Kept<31> match_kept(c, match);
    Kept<30> player_kept(c, player);
    constexpr std::uint32_t flags_of = 0x10289560, side_of = 0x10283690;
    c.set(3, player);
    c.call(flags_of, 0x101f52a4);
    if (compare(c, std::int16_t(c.r(3)), 1) == 0) {
        c.set(3, player);
        c.call(side_of, 0x101f52b8);
        if (compare(c, s8(c.r(3)), 1) == 0) return result(c, c.u8(match + 0x7923));
        c.set(3, player);
        c.call(side_of, 0x101f52d4);
        if (compare(c, s8(c.r(3)), 0) == 0) return result(c, c.u8(match + 0x7922));
    }
    c.set(3, player);
    c.call(side_of, 0x101f52f0);
    // The first side's holder is at +0xe008, the second side's at +0xe004: equality via cntlzw.
    const bool first = compare(c, s8(c.r(3)), 1) == 0;
    const auto held = c.u32(match + (first ? 0xe008 : 0xe004));
    return result(c, u(s8(held == player)));
}
}
