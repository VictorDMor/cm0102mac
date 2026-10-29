#include "match_pl_ball_action_detail.hpp"

namespace cm::aot::match_pl::detail {
using namespace guest;

void BallAction::initial_condition_draw() {
    const auto source_column = c.r(4), source_row = c.r(5), control = c.r(7), target_row = c.r(10);
    const auto first = fp.fctiwz(1, c.f(31)); c.put8(at(0x16f), source_column);
    const auto second = fp.fctiwz(0, c.f(30)); c.put16(at(0xee), UINT32_MAX);
    c.put64(at(0xe4), first); c.put64(at(0xdc), second);
    c.put8(at(0x173), source_row); c.put32(at(0x178), control); c.put8(at(0x187), target_row);
    const auto quotient = [&](unsigned a, unsigned b, bool retain_deficit = false) {
        const auto x = std::uint32_t(10000 - c.i16(at(a + 6))), y = std::uint32_t(10000 - c.i16(at(b + 6)));
        if (retain_deficit) c.set(4, x);
        c.s.wr<1>(reg::xer_ca, std::uint32_t(std::int32_t(c.i16(at(b + 6)))) <= 10000);
        const auto product = x * y; shifted_sign(high(product, 0x14f8b589), 12);
        return std::uint32_t(std::int32_t(product) / 50000);
    };
    const auto bound = quotient(0xe4, 0xdc, true); c.set(3, unsigned(std::int32_t(bound) < 0));
    if (compare(c, std::int32_t(bound)) == 0) { c.set(13, 0); return; }
    advance(0x10247108);
    // Signed-halfword inputs bound the quotient to[-19473,36582]. Both
    // comparisons remain, but the original wide sampler cannot be reached.
    const auto check = [&](std::int32_t limit) {
        c.put64(at(0xdc), fp.fctiwz(1, c.f(31))); c.put64(at(0xe4), fp.fctiwz(0, c.f(30)));
        c.set(5, 0x14f8b589); c.set(3, std::uint32_t(limit));
        const auto value = quotient(0xdc, 0xe4); c.set(4, unsigned(std::int32_t(value) < 0)); compare(c, std::int32_t(value), limit);
    };
    check(65535); check(-65535);
    const auto a = fp.fctiwz(0, c.f(31)); c.set(3, c.u32(cursor));
    const auto b = fp.fctiwz(2, c.f(30)), d = fp.fctiwz(1, c.f(31));
    c.set(7, 0x14f8b589); c.set(5, 0x14f8b589); c.put64(at(0x94), a);
    const auto e = fp.fctiwz(0, c.f(30)); c.set(8, c.u16(c.r(3))); c.put64(at(0x9c), b);
    c.set(3, std::uint32_t(10000 - c.i16(at(0x9a)))); c.set(4, c.u32(seed));
    c.put64(at(0xa4), d); c.put64(at(0xac), e);
    const auto product = c.r(3) * std::uint32_t(10000 - c.i16(at(0xa2)));
    const auto word = c.r(8) + c.r(4), divisor = std::uint32_t(std::int32_t(product) / 50000);
    shifted_sign(high(product, 0x14f8b589), 12); c.set(7, divisor);
    c.set(4, std::uint32_t(10000 - c.i16(at(0xaa)))); c.set(3, std::uint32_t(10000 - c.i16(at(0xb2))));
    c.s.wr<1>(reg::xer_ca, std::uint32_t(std::int32_t(c.i16(at(0xb2)))) <= 10000);
    const auto sign_product = high(c.r(4) * c.r(3), 0x14f8b589); shifted_sign(sign_product, 12);
    c.set(6, std::uint32_t(divide_signed<4>(word, divisor))); c.set(5, c.r(6) * divisor);
    c.set(3, std::uint32_t(sign_product >> 12)); c.set(4, unsigned(sign_product < 0));
    const auto residual = word - c.r(5); c.set(13, compare(c, std::int32_t(c.r(3) + c.r(4))) > 0 ? residual : 0u - residual);
}

void BallAction::initial_distance_draw() {
    c.put8(at(0x41), 0); c.put8(at(0x45), 0); c.set(30, std::uint32_t(std::int32_t(std::int16_t(c.r(13)))));
    c.set(14, 0); c.put8(at(0xed), 0); c.put8(at(0xec), 0);
    leave(c, {{3, match()}, {4, c.u8(at(0x16f))}, {5, c.u8(at(0x173))}, {6, c.r(26)}, {7, c.u8(at(0x187))}});
    c.call(0x101ef8d0, 0x102474a0); c.set_f(2, c.f32(singles + 0x34)); c.set_f(31, c.f(1));
    c.set_f(0, c.f32(player() + 0xc8)); fp.fcmp(0, c.f(2), c.f(0));
    c.set_f(30, c.cr(0) & 4 ? c.f32(singles + 0x34) : c.f32(player() + 0xc8));
    const auto bound = [&](unsigned destination) {
        c.set_f(destination, c.f32(singles + 0x84)); c.set_f(destination, fp.fmuls(c.f(destination), metric()));
        c.set_f(destination, fp.fmuls(c.f(destination), metric())); c.set_f(destination, fp.fmuls(metric(), c.f(destination)));
        c.set_f(destination, fp.fmuls(metric(), c.f(destination))); c.set_f(destination, fp.fdivs(c.f(destination), c.f(30)));
    };
    bound(1); c.set_f(0, c.f32(singles)); fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 2) c.set(13, 0);
    else {
        advance(0x1024750c); bound(0); c.set(3, 65535); c.put64(at(0x94), fp.fctiwz(0, c.f(0)));
        bool narrow = compare(c, c.i32(at(0x98)), 65535) <= 0;
        if (narrow) {
            bound(0); c.set(3, std::uint32_t(-65535)); c.put64(at(0x94), fp.fctiwz(0, c.f(0)));
            narrow = compare(c, c.i32(at(0x98)), -65535) >= 0;
        }
        if (narrow) {
            c.set_f(1, c.f32(singles + 0x84)); c.set_f(0, c.f32(singles + 0x84));
            c.set_f(1, fp.fmuls(c.f(1), metric())); c.set(3, c.u32(cursor)); c.set_f(0, fp.fmuls(c.f(0), metric()));
            c.set(3, c.u16(c.r(3))); const auto word = c.r(3) + c.u32(seed);
            for (int i = 0; i < 3; ++i) {
                c.set_f(1, i == 0 ? fp.fmuls(c.f(1), metric()) : fp.fmuls(metric(), c.f(1)));
                c.set_f(0, i == 0 ? fp.fmuls(c.f(0), metric()) : fp.fmuls(metric(), c.f(0)));
            }
            c.set(5, word); c.set_f(1, fp.fdivs(c.f(1), c.f(30))); c.put64(at(0x94), fp.fctiwz(1, c.f(1)));
            c.set_f(0, fp.fdivs(c.f(0), c.f(30))); c.set(4, c.u32(at(0x98)));
            // Original4773c's undefined quotient for a rounded-zero bound is
            // multiplied byzero and killed before any call. Choosezero there.
            const auto q = c.r(4) ? std::uint32_t(divide_signed<4>(word, c.r(4))) : 0u;
            c.set(3, q); c.put64(at(0x9c), fp.fctiwz(0, c.f(0))); const auto positive = compare(c, c.i32(at(0xa0))) > 0;
            const auto residual = word - q * c.r(4); c.set(13, positive ? residual : 0u - residual);
        } else {
            bound(1); bound(0); c.set(7, sample()); c.set(6, 0x80008001); c.set(5, 65535); c.set(3, 0x80008001);
            c.put32(at(0x9c), 0x43300000); c.put32(at(0xa4), 0x43300000);
            const auto word = c.r(7);
            const auto product = std::int32_t(std::uint32_t(high(word, std::int32_t(0x80008001))) + word);
            c.set(4, shifted_sign(product, 15)); c.set(13, word - std::uint32_t(std::int32_t(word) / 65535) * 65535u);
            c.put32(at(0xa8), c.r(13) ^ 0x80000000); c.set_f(2, c.f64(doubles + 0x80)); c.set_f(4, c.f64(doubles + 0x80));
            c.set_f(3, c.f64(doubles)); c.put64(at(0x94), fp.fctiwz(1, c.f(1))); c.put64(at(0xb4), fp.fctiwz(0, c.f(0)));
            c.set_f(1, fp.fsub(c.f64(at(0xa4)), c.f(2))); c.put32(at(0xa0), c.u32(at(0x98)) ^ 0x80000000);
            c.set_f(2, c.f64(at(0x9c))); c.set_f(0, fp.fsub(c.f(2), c.f(4))); c.set_f(0, fp.fdiv(c.f(0), c.f(3)));
            const auto reduced = c.u32(at(0xb8)) * 10; shifted_sign(std::int32_t(std::uint32_t(high(reduced, std::int32_t(0x80008001))) + reduced), 15);
            c.set(3, std::uint32_t(std::int32_t(reduced) / 65535)); c.set_f(0, fp.fmul(c.f(0), c.f(1)));
            c.put64(at(0xac), fp.fctiwz(0, c.f(0))); c.set(13, c.u32(at(0xb0)));
            enter(c, 0x10247674, {c.r(3)}); sample_range(c, c.r(3));
            const auto correction = c.r(3) + 5; bound(0); shifted_sign(high(correction, 0x66666667), 2);
            c.set(13, c.r(13) + std::uint32_t(std::int32_t(correction) / 10)); c.put64(at(0xbc), fp.fctiwz(0, c.f(0)));
            c.set(3, c.u32(at(0xc0)));
            if (compare(c, std::int32_t(c.r(13)), std::int32_t(c.r(3) - 1)) > 0) {
                bound(0); c.put64(at(0xc4), fp.fctiwz(0, c.f(0))); c.set(3, c.u32(at(0xc8))); c.set(13, c.r(3) - 1);
            }
        }
    }
    c.set(15, std::uint32_t(std::int32_t(std::int16_t(c.r(13)))));
    if (compare_logical(c, receiver()) == 0) {
        range(std::int16_t(c.r(15)), 13, 0x102477a4, 0x1024788c, true, true);
        c.set(15, std::uint32_t(std::int32_t(std::int16_t(c.r(13)))));
    }
    c.set_f(0, c.f64(doubles + 0xc8)); fp.fcmp(0, metric(), c.f(0));
    if ((c.cr(0) & 8) && compare(c, kind(), 61) != 0 && compare(c, kind(), 92) != 0) {
        c.set_f(1, c.f32(singles + 0xf4)); c.set_f(0, c.f32(singles + 0xf8));
        c.set_f(0, fp.multiply_add(0x1024792c, c.f(1), metric(), c.f(0))); c.put64(at(0x94), fp.fctiwz(0, c.f(0)));
        c.set(15, c.r(15) - c.u32(at(0x98)));
    }
    const auto duration = [&](unsigned slot) {
        c.set_f(0, c.f32(singles + 0x8c)); c.set_f(1, c.f64(doubles + 0xc0));
        c.set_f(0, fp.fmuls(c.f(0), metric())); c.set_f(0, fp.fadd(c.f(1), c.f(0))); c.put64(at(slot), fp.fctiwz(0, c.f(0)));
        const auto value = std::int8_t(c.u32(at(slot + 4)));
        const auto product = high(std::uint32_t(std::int32_t(value)), 0x66666667);
        c.set(3, shifted_sign(product, 1)); return int(value) / 5 * 5;
    };
    auto duration_value = duration(0x94);
    if (compare(c, duration_value, 1) < 0) duration_value = 1; else duration_value = duration(0x9c);
    c.set(3, std::uint32_t(duration_value)); c.set(27, std::uint32_t(std::int32_t(std::int8_t(duration_value))));
    if (compare(c, kind(), 54) != 0 && compare(c, kind(), 92) != 0 && compare(c, kind(), 61) != 0)
        c.put8(match() + std::uint32_t(std::int32_t(std::int8_t(actor.side()))) + 0x1be, 1);
    c.set_f(0, c.f64(doubles + 0x10)); c.set_f(1, fp.fadd(c.f(0), metric())); c.set_f(1, fp.frsp(c.f(1)));
    enter(c, 0x10247a34, {player(), std::uint32_t(std::int32_t(kind())), c.u8(at(0x16f)), c.u8(at(0x173)), c.r(26), c.u8(at(0x187))});
    c.set(10, at(0x48)); turn_direction(c, player(), kind(), c.f(1), at(0x48));
    c.set(16, std::uint32_t(std::int32_t(c.i16(at(0x48))))); range(std::int32_t(c.r(16)), 31, 0x10247a68, 0x10247b40, true);
    c.put16(at(0x48), c.r(31));
}

void BallAction::calculate_quality() {
    const auto role = [&](std::uint32_t back) { enter(c, back, {player()}); current_role(c, player()); return c.r(3); };
    const auto goal_line = [&](std::uint32_t back) {
        enter(c, back, {player()}); on_goal_line(c, player()); return compare(c, std::int8_t(c.r(3))) != 0;
    };
    const auto roles_masked = [&](std::uint32_t back) {
        enter(c, back, {player()}); both_roles_masked(c, player()); return compare(c, std::int8_t(c.r(3))) != 0;
    };
    const auto club_instruction = [&](std::uint32_t back) {
        if (compare_logical(c, c.u32(player() + 0xfc)) == 0) return false;
        leave(c, {{3, c.u32(player() + 0xfc)}, {4, 0}, {5, 1}, {6, 0}, {7, 0}}); c.call(0x1014c2e0, back);
        return compare(c, std::int8_t(c.r(3))) != 0;
    };
    const auto supported_delivery = [&](bool clearance) {
        const auto skill = [&] {
            const auto offset = clearance ? 0x12eu : 0x11eu + std::uint32_t(int(kind()) * 2);
            return int(c.i16(player() + offset));
        };
        bool advanced = goal_line(clearance ? 0x10247c40 : 0x10247eb0);
        if (advanced) advanced = roles_masked(clearance ? 0x10247c50 : 0x10247ec0);
        if (advanced) advanced = compare(c, int(role(clearance ? 0x10247c60 : 0x10247ed0) & 0x38)) != 0;
        if (advanced) {
            enter(c, clearance ? 0x10247c70 : 0x10247ee0, {player()}); alternate_strategy_flags(c, player());
            c.set(6, 0); c.set(5, c.r(3) & 4); c.set(4, 0); c.set(3, 0); advanced = compare(c, int(c.r(5))) != 0;
        }
        if (advanced) {
            const auto bonus = club_instruction(clearance ? 0x10247cbc : 0x10247f2c) ? 75 : 62;
            set_score(skill() - c.i16(at(0x48)) + bonus); return;
        }
        if (goal_line(clearance ? 0x10247cfc : 0x10247f84) && roles_masked(clearance ? 0x10247d0c : 0x10247f94)) {
            const auto bonus = club_instruction(clearance ? 0x10247d38 : 0x10247fc0) ? 62 : 45;
            set_score(skill() - c.i16(at(0x48)) + bonus); return;
        }
        bool forward = goal_line(clearance ? 0x10247d78 : 0x10248018);
        if (!forward) {
            enter(c, clearance ? 0x10247d88 : 0x10248028, {player()}); in_attacking_third(c, player());
            if (compare(c, std::int8_t(c.r(3))) != 0)
                forward = compare(c, int(std::uint16_t(role(clearance ? 0x10247d98 : 0x10248038)) & 0x880)) != 0;
        }
        if (forward) set_score(skill() - c.i16(at(0x48)) + 25);
        else {
            const auto penalty = std::int16_t(c.r(15)); c.set(3, std::uint32_t(high(std::uint32_t(std::int32_t(penalty)), 0x55555556)));
            const auto attribute = skill(), turning = int(c.i16(at(0x48)));
            c.set(5, std::uint32_t(attribute)); c.set(4, std::uint32_t(turning)); set_score(attribute - int(penalty) / 3 - turning);
        }
    };
    switch (kind()) {
    case 61: case 91: case 92: set_score(c.u16(player() + 0x198) - c.r(15)); break;
    case 93: set_score(c.u16(player() + 0x134) - c.r(15) - std::uint32_t(c.i16(at(0x48)))); break;
    case 52: case 55:
        c.set(4, std::uint32_t(std::int32_t(c.i16(player() + 0x186)))); c.put8(at(0xed), 1);
        set_score(std::int32_t(c.r(4)) - int(std::int16_t(c.r(15))) / 5 + 300); break;
    case 90: c.put8(at(0xed), 1); set_score(c.u16(player() + 0x15a) - c.r(15) + 300); break;
    case 54:
        c.set(4, std::uint32_t(std::int32_t(c.i16(player() + 0x18a)))); c.put8(at(0xed), 1); set_score(c.r(4) - c.r(15) * 2); break;
    case 94: supported_delivery(true); break;
    case 6: case 7: case 8: supported_delivery(false); break;
    default:
        c.set(4, player() + std::uint32_t(int(kind()) * 2));
        set_score(c.u16(c.r(4) + 0x11e) - c.r(15) - std::uint32_t(c.i16(at(0x48)))); break;
    }
    apply_weather();
    range(score(), 13, 0x102483ac, 0x10248494, true, true);
    range(std::int16_t(c.r(30)), 15, 0x10248534, 0x1024861c, true, true);
    set_score(c.r(13) - c.r(15) + std::uint32_t(c.i16(player() + 0x2a)) + std::uint32_t(c.i16(player() + 0x1a0)));
}

void BallAction::apply_weather() {
    if (compare(c, c.u8(match() + 0x791f), 5) != 0) return;
    c.set_f(0, c.f64(doubles + 0x278)); fp.fcmp(0, metric(), c.f(0)); if (!(c.cr(0) & 4)) return;
    unsigned divisor, factor; std::uint32_t back;
    switch (c.i8(match() + 0x30d8)) {
    case 1: divisor = 7; factor = 0x148; back = 0x10248134; break;
    case 2: divisor = 5; factor = 0x1b8; back = 0x102481d4; break;
    case 3: divisor = 3; factor = 0x1a8; back = 0x10248270; break;
    case 4: divisor = 3; factor = 0x1e0; back = 0x10248308; break;
    default: return;
    }
    advance(back); if (compare(c, modulo(divisor)) != 0) return;
    c.set(3, std::uint32_t(std::int32_t(score())) ^ 0x80000000); c.put32(at(0x94), 0x43300000);
    c.set_f(1, c.f64(doubles + 0x80)); c.put32(at(0x98), c.r(3)); c.set_f(2, c.f64(doubles + factor));
    c.set_f(0, fp.fsub(c.f64(at(0x94)), c.f(1))); c.set_f(0, fp.fmul(c.f(2), c.f(0))); c.put64(at(0x9c), fp.fctiwz(0, c.f(0)));
    // The source multiplies the score by its converted scaled value. Keep
    // this original quirk rather than replacing it with a linear scaling.
    set_score(c.r(31) * c.u32(at(0xa0)));
}

void BallAction::query_opponent() {
    if (compare(c, c.u8(match() + 0x791f), 4) != 0 && compare(c, byte(0xed)) == 0) {
        c.put32(at(0x38), at(0x44)); c.put32(at(0x3c), 0);
        leave(c, {{3, match()}, {4, player()}, {5, actor.side()}, {6, 108},
                  {7, at(0x47)}, {8, at(0x46)}, {9, at(0x4a)}, {10, at(0x45)}});
        c.call(0x101eaf60, 0x102486f4); c.set(13, c.r(3));
    } else c.set(13, 0);
    leave(c, {{3, match()}, {4, player()}, {5, 108}, {6, opponent()}, {7, std::uint32_t(std::int32_t(byte(0x45)))}, {8, 0}});
    c.call(0x101f5350, 0x10248720);
}
}
