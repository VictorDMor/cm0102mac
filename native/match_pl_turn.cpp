#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;

std::uint32_t can_cover_goal(Cpu& c, std::uint32_t player, std::uint32_t column, std::uint32_t row) {
    Frame frame(c, 0x10281780, 0x60, {.first_gpr = 27, .gpr_offset = -0x24, .first_fpr = 31});
    keep(c, {{29, column}, {28, player}, {30, row}});
    c.set(3, std::uint32_t(std::int32_t(std::int8_t(column))));
    bool same = compare(c, std::int8_t(column), c.i8(player + 0x105)) == 0;
    if (same) { c.set(3, std::uint32_t(std::int32_t(std::int8_t(row)))); same = compare(c, std::int8_t(row), c.i8(player + 0x106)) == 0; }
    if (same) return result(c, 1);
    const auto goal_row = compare(c, c.i8(player + 0x27)) == 0 ? 0u : 11u;
    keep(c, {{31, goal_row}});
    leave(c, {{3, c.u32(player + 0x1a4)}, {4, column}, {5, row}, {6, 4}, {7, goal_row}});
    c.call(0x101ee610, goal_row == 0 ? 0x102817f8 : 0x1028181c);
    keep(c, {{27, c.r(3)}});
    leave(c, {{3, c.u32(player + 0x1a4)}, {4, column}, {5, row}, {6, c.u8(player + 0x105)}, {7, c.u8(player + 0x106)}});
    c.call(0x101ee610, 0x1028183c);
    const auto angle = std::int16_t(c.r(27)) - std::int16_t(c.r(3));
    keep(c, {{27, c.u8(player + 0x10f)}});
    c.set(3, std::uint32_t(angle)); c.call(0x10775540, 0x10281850);
    c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(27)))));
    if (compare(c, std::int32_t(c.r(3)), std::int8_t(c.r(27)) + 10) >= 0) return result(c, 0);
    c.set(3, std::uint32_t(std::int8_t(column) - c.i8(player + 0x105))); c.call(0x10775540, 0x10281874);
    keep(c, {{27, std::uint32_t(std::int32_t(std::int8_t(c.r(3))))}});
    c.set(4, std::uint32_t(std::int32_t(std::int8_t(row))));
    c.set(3, std::uint32_t(c.i8(player + 0x106) - std::int8_t(row))); c.call(0x10775540, 0x1028188c);
    const auto distance_row = std::uint32_t(std::int8_t(c.r(3)) * 36);
    const auto table = c.toc(-0x6424);
    c.set(8, table); c.set_f(31, c.f32(table + distance_row + std::uint32_t(std::int8_t(c.r(27)) * 4)));
    leave(c, {{3, c.u32(player + 0x1a4)}, {4, column}, {5, row}, {6, 4}, {7, goal_row}});
    c.call(0x101ef8d0, 0x102818c0);
    Fpu(c).fcmp(0, c.f(31), c.f(1));
    c.set_cr(0, (c.cr(0) & ~2u) | ((c.cr(0) & (8u | 2u)) ? 2u : 0u));
    return result(c, (c.cr(0) & 2) ? 1 : 0);
}

std::uint32_t turn_direction(Cpu& c, std::uint32_t player, std::int16_t action, double duration, std::uint32_t penalty) {
    Frame frame(c, 0x102818f0, 0x60, {.first_gpr = 29, .gpr_offset = -0x1c, .first_fpr = 31});
    c.set_f(31, duration); c.set(9, 0);
    const auto singles = c.toc(-0x51e0);
    keep(c, {{31, singles}, {30, penalty}, {29, player}});
    c.set(3, 2); c.put16(penalty, 0);
    for (int excluded : {91, 92, 61, 54, 52, 55, 90, 51, 53, 57, 3})
        if (compare(c, action, excluded) == 0) return 2;
    leave(c, {{3, c.u32(player + 0x1a4)}, {4, c.r(5)}, {5, c.r(6)}, {6, c.r(7)}, {7, c.r(8)}});
    c.call(0x101ee610, 0x102819c4);
    const auto difference = std::int16_t(std::int16_t(c.r(3)) - c.i16(player + 0x19e));
    c.set(4, std::uint32_t(std::int32_t(c.i16(player + 0x19e))));
    const auto sign = compare(c, difference) < 0 ? -1 : 1;
    c.set(3, std::uint32_t(sign));
    auto magnitude = std::int16_t(int(difference) * sign);
    if (compare(c, magnitude, 180) > 0) { c.s.wr<1>(reg::xer_ca, magnitude <= 360); magnitude = std::int16_t(360 - magnitude); }
    const auto turn = std::int16_t(int(magnitude) * sign);
    Fpu fp(c);
    c.set_f(2, fp.fmuls(c.f32(singles + 0x168), duration));
    c.set_f(1, fp.fdivs(c.f(2), c.f32(singles + 0x3c)));
    c.set_f(3, c.f32(singles + 0x168));
    const auto threshold = fp.fsubs(c.f(3), fp.fsubs(c.f(1), c.f32(player + 0xe8)));
    c.put64(frame.at(0x38), fp.fctiwz(0, threshold));
    const auto raw = c.u32(frame.at(0x3c));
    const auto limit = std::int16_t(raw), negative = std::int16_t(0u - raw);
    leave(c, {{3, std::uint32_t(std::int32_t(turn))}, {4, std::uint32_t(std::int32_t(negative))},
              {5, std::uint32_t(std::int32_t(turn))}, {6, raw}});
    bool inside = compare(c, turn, limit) <= 0;
    if (inside) inside = compare(c, turn, negative) >= 0;
    if (inside) return result(c, c.u8(player + 0x2d));
    if (compare(c, turn, limit) > 0) {
        const auto profile = c.u32(player + 0x70); c.set(4, profile);
        c.put16(penalty, std::uint32_t((20 - c.i8(profile + 0x30)) * 12));
        c.s.wr<1>(reg::xer_ca, std::uint32_t(std::int32_t(c.i8(profile + 0x30))) <= 20);
        return result(c, 0);
    }
    // Outside [negative,limit], after excluding turn>limit, turn<negative.
    // Retain the original final comparison, without its unreachable fallback.
    compare(c, turn, negative);
    const auto profile = c.u32(player + 0x70); c.set(4, profile);
    c.put16(penalty, std::uint32_t((20 - c.i8(profile + 0x3b)) * 12));
    c.s.wr<1>(reg::xer_ca, std::uint32_t(std::int32_t(c.i8(profile + 0x3b))) <= 20);
    return result(c, 1);
}
}
