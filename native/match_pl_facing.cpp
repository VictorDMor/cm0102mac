#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;

std::uint32_t accepts_target(Cpu& c, std::uint32_t player, std::uint32_t column, std::uint32_t row,
                            std::int8_t adaptive, std::uint32_t feedback, double time, double reach) {
    Frame frame(c, 0x10244c60, 0xa0, {.first_gpr = 19, .first_fpr = 30});
    c.set_f(31, reach); c.set_f(30, time);
    const auto doubles = c.toc(-0x51e8), singles = c.toc(-0x51e0);
    const auto seed = c.toc(-0x7b90), begin = c.toc(-0x7b8c), end = c.toc(-0x7b88), cursor = c.toc(-0x7b84);
    keep(c, {{27, column}, {26, player}, {28, row}, {30, doubles}, {29, feedback},
             {31, seed}, {22, begin}, {23, end}, {24, cursor}, {25, singles}});
    const auto dx = std::int8_t(column - c.u8(player + 0x105)), dy = std::int8_t(row - c.u8(player + 0x106));
    const auto directions = c.toc(-0x641c), direction_at = directions + std::uint32_t(dx * 48 + dy * 2);
    const auto direction = c.i16(direction_at + 0x1c8);
    keep(c, {{21, std::uint32_t(std::int32_t(direction))}});
    leave(c, {{3, std::uint32_t(std::int32_t(c.i16(player + 0x19e)))}, {4, direction_at}, {5, directions}, {7, std::uint32_t(std::int32_t(dy))}});
    if (compare(c, c.i16(player + 0x19e), direction) == 0) return result(c, 1);
    Fpu fp(c);
    std::uint32_t tolerance = 75;
    if (compare(c, adaptive) != 0) {
        leave(c, {{3, c.u32(player + 0x1a4)}, {4, player}, {5, column}, {6, row}});
        c.call(0x101eeac0, 0x10244d14);
        const bool approaching = compare(c, std::int8_t(c.r(3))) != 0;
        const auto raw_trait = c.u8(player + 0x10e); const auto trait = std::int8_t(raw_trait);
        c.set(3, raw_trait);
        const auto conversion = approaching ? 0x48u : 0x40u, output = approaching ? 0x40u : 0x48u;
        c.put32(frame.at(conversion), 0x43300000);
        c.put32(frame.at(conversion + 4), std::uint32_t(trait * (approaching ? 4 : 5)) ^ 0x80000000);
        c.set_f(1, c.f64(doubles + 0x80));
        c.set_f(2, fp.multiply_add(approaching ? 0x10244d44 : 0x10244d84,
            c.f32(singles + 0x5c), c.f(30), c.f32(singles + (approaching ? 0xe8 : 0x80))));
        const auto scaled = fp.fadds(c.f(2), fp.fsubs(c.f64(frame.at(conversion)), c.f(1)));
        c.put64(frame.at(output), fp.fctiwz(0, scaled)); tolerance = c.u32(frame.at(output + 4));
        keep(c, {{20, tolerance}});
        c.set(3, std::uint32_t(c.i8(player + 0x105) - std::int8_t(column))); c.call(0x10775540, 0x10244dbc);
        keep(c, {{19, std::uint32_t(std::int32_t(std::int8_t(c.r(3))))}});
        c.set(4, std::uint32_t(std::int32_t(std::int8_t(row))));
        c.set(3, std::uint32_t(c.i8(player + 0x106) - std::int8_t(row))); c.call(0x10775540, 0x10244dd4);
        const auto distance_row = std::int8_t(c.r(3)); c.set(3, std::uint32_t(distance_row * 36));
        const auto table = c.toc(-0x6424); c.set(4, table);
        c.set_f(1, c.f32(singles + 0xf0));
        c.set_f(2, fp.fsubs(c.f32(table + std::uint32_t(distance_row * 36 + std::int8_t(c.r(19)) * 4)), c.f(31)));
        const auto adjustment = fp.multiply_add(0x10244dfc, c.f(1), c.f(2), c.f32(singles + 0xec));
        c.put64(frame.at(0x40), fp.fctiwz(0, adjustment)); tolerance += c.u32(frame.at(0x44));
        keep(c, {{20, tolerance}});
        if (compare(c, std::int16_t(tolerance), 30) < 0) tolerance = 30;
        else if (compare(c, std::int16_t(tolerance), 135) > 0) tolerance = 135;
    }
    keep(c, {{20, tolerance}});
    c.set(3, std::uint32_t(c.i16(player + 0x19e) - int(direction))); c.call(0x10775540, 0x10244e4c);
    auto angle = std::int16_t(c.r(3));
    if (compare(c, angle, 180) > 0) { c.s.wr<1>(reg::xer_ca, angle <= 360); angle = std::int16_t(360 - angle); }
    c.set(3, std::uint32_t(std::int32_t(angle)));
    if (compare(c, angle, std::int16_t(tolerance)) >= 0) return result(c, 0);
    if (compare_logical(c, feedback) == 0) return result(c, 1);
    leave(c, {{3, c.u32(player + 0x1a4)}, {4, c.u8(player + 0x105)}, {5, c.u8(player + 0x106)}, {6, column}, {7, row}});
    c.call(0x101eeeb0, 0x10244e90); c.put8(feedback, c.r(3)); c.set(3, std::uint32_t(std::int32_t(c.i8(feedback))));
    const auto trait = c.u8(player + 0x10e); const auto magnitude = int(c.i8(feedback)) * c.i8(feedback);
    keep(c, {{25, trait}, {26, std::uint32_t(magnitude)}});
    std::int32_t draw = 0;
    if (compare(c, magnitude) != 0) {
        const auto next = c.u32(cursor) + 2; c.set(3, next);
        const auto ordering = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
        if (ordering > 0) { c.put32(cursor, begin); c.call(0x1077b770, 0x10244ed8); c.put32(seed, c.r(3) & 0xffff); }
        // Squaring a signed-byte feedback bounds this draw by 16384; the
        // expanded wide arm and recursive sample_range call are unreachable.
        c.set(3, std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed)); compare(c, magnitude);
        draw = std::int32_t(c.r(3)) % magnitude;
    }
    keep(c, {{19, std::uint32_t(draw)}});
    return result(c, compare(c, std::int8_t(trait), draw) >= 0);
}
}
