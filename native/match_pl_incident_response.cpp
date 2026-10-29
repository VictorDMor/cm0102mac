#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void resolve_incident_response(Cpu& c, std::uint32_t player, std::uint32_t policy) {
    Frame frame(c, 0x102884d0, 0x90, {.first_gpr = 21});
    const auto doubles = c.toc(-0x51e8), singles = c.toc(-0x51e0), seed = c.toc(-0x7b90),
               begin = c.toc(-0x7b8c), end = c.toc(-0x7b88), cursor = c.toc(-0x7b84);
    keep(c, {{23, doubles}, {24, singles}, {25, 0}, {26, end}, {27, begin},
             {28, seed}, {29, policy}, {30, cursor}, {31, player}});
    c.put8(frame.at(0x38), 0);
    const Player actor{c, player};
    Fpu fp(c);
    const auto advance = [&](std::uint32_t back) {
        const auto next = c.u32(cursor) + 2; c.set(3, next);
        const auto order = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
        if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
    };
    const auto sample = [&] { return std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed); };
    const auto high_product = [](std::uint32_t value, std::int32_t multiplier) {
        return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * multiplier) >> 32);
    };
    const auto shifted_sign = [&](std::int32_t high, unsigned bits) {
        c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << bits) - 1)));
        return std::uint32_t(high < 0);
    };
    const auto respond = [&](unsigned kind, std::uint32_t back) {
        enter(c, back, {player, kind, 0}); accepts_response(c, player, kind, 0);
        return compare(c, std::int8_t(c.r(3))) != 0;
    };
    const auto message = [&](unsigned id, std::uint32_t back, unsigned style = 0) {
        const auto match = actor.match();
        leave(c, {{3, c.u32(match + 0x79f4)}, {4, id}, {5, match},
                  {6, style == 1 ? 0u : style == 2 ? UINT32_MAX : c.u8(player + 0x2f)},
                  {7, style == 1 ? 0u : style == 2 ? unsigned(actor.side() == 0) : actor.side()},
                  {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}});
        c.call(0x10201180, back);
    };
    const auto request = [&](unsigned kind, std::uint32_t back) {
        leave(c, {{3, actor.match()}, {4, player}, {5, frame.at(0x38)}, {6, kind}, {7, c.r(29)}});
        c.call(0x101f51e0, back);
    };
    const auto draw_resistance = [&](bool first) {
        const auto input = first ? 0x4cu : 0x3cu, output = first ? 0x3cu : 0x4cu;
        const auto bound_register = first ? 21u : 22u, draw_register = first ? 22u : 21u;
        c.set_f(1, c.f32(singles + 0xf0));
        c.put64(frame.at(input), fp.fctiwz(0, fp.fsubs(c.f(1), c.f32(player + 0x78))));
        const auto bound_word = c.u32(frame.at(input + 4)); const auto bound = std::int32_t(bound_word);
        c.set(bound_register, bound_word);
        std::uint32_t draw = 0;
        if (compare(c, bound) != 0) {
            advance(first ? 0x10288568 : 0x102887a8);
            if (compare(c, bound, 65535) <= 0 && compare(c, bound, -65535) >= 0) {
                c.set(3, sample()); compare(c, bound);
                const auto residual = std::int32_t(std::int64_t(std::int32_t(c.r(3))) % bound);
                draw = bound > 0 ? std::uint32_t(residual) : 0u - std::uint32_t(residual);
            } else {
                c.put32(frame.at(input), 0x43300000); c.put32(frame.at(input + 4), bound_word ^ 0x80000000);
                c.set_f(2, c.f64(doubles + 0x80)); c.set_f(1, fp.fsub(c.f64(frame.at(input)), c.f(2)));
                c.set_f(2, fp.fdiv(c.f(1), c.f64(doubles)));
                const auto word = c.u16(c.u32(cursor)); const auto value = std::uint32_t(word) + c.u32(seed);
                const auto residual = std::int32_t(value) % 65535;
                c.put32(frame.at(0x44), 0x43300000); c.put32(frame.at(0x48), std::uint32_t(residual) ^ 0x80000000);
                c.set_f(1, c.f64(doubles + 0x80));
                c.put64(frame.at(output), fp.fctiwz(0, fp.fmul(c.f(2), fp.fsub(c.f64(frame.at(0x44)), c.f(1)))));
                const auto whole = c.u32(frame.at(output + 4)); c.set(draw_register, whole);
                const auto multiplied = bound_word * 10u, reduced = std::uint32_t(std::int32_t(multiplied) / 65535);
                const auto partial = std::int32_t(std::uint32_t(high_product(multiplied, std::int32_t(0x80008001))) + multiplied);
                shifted_sign(partial, 15);
                leave(c, {{3, reduced}, {4, std::uint32_t(residual) ^ 0x80000000}, {5, 65535},
                          {6, std::uint32_t(std::int32_t(value) < 0)}, {7, value}, {8, word}});
                enter(c, first ? 0x10288640 : 0x10288880, {reduced}); sample_range(c, reduced);
                const auto numerator = std::int32_t(c.r(3) + 5), high = high_product(std::uint32_t(numerator), 0x66666667);
                c.set(4, shifted_sign(high, 2)); c.set(3, std::uint32_t(numerator / 10));
                draw = whole + c.r(3);
                if (compare(c, std::int32_t(draw), std::int32_t(bound_word - 1)) > 0) draw = bound_word - 1;
            }
        }
        c.set(draw_register, draw);
        if (compare(c, std::int32_t(draw)) != 0) return true;
        c.set_f(1, c.f32(player + 0x78)); c.set_f(0, c.f64(doubles + 0x18)); fp.fcmp(0, c.f(1), c.f(0));
        return (c.cr(0) & 8) != 0;
    };
    if (respond(5, 0x10288518)) {
        if (draw_resistance(true)) {
            c.set(25, 1); message(0x20e6, 0x102886e8); request(3, 0x10288700);
        } else { message(0x20db, 0x1028872c); request(5, 0x10288744); }
    } else if (respond(4, 0x10288758)) {
        if (draw_resistance(false)) {
            c.set(25, 1); message(0x20e6, 0x10288928); request(3, 0x10288940);
        } else { message(0x20da, 0x1028896c); request(4, 0x10288984); }
    } else if (respond(2, 0x10288998)) {
        c.set(25, 1); message(0x20e6, 0x102889cc); request(3, 0x102889e4);
    } else if (respond(2, 0x102889f8)) {
        // These two kind2 requests are separate draws in the original.
        message(0x20d8, 0x10288a28); request(2, 0x10288a40);
    } else if (respond(1, 0x10288a54)) {
        message(0x20d8, 0x10288a84); request(1, 0x10288a9c);
    }
    c.set(29, UINT32_MAX);
    auto choice = c.i8(frame.at(0x38));
    unsigned effect = 0; std::uint32_t back = 0;
    if (compare(c, choice, 4) == 0) { effect = 8; c.set(29, 0x1ff8); back = 0x10288b14; }
    else if (choice > 4) {
        if (compare(c, choice, 9) == 0) { effect = 0x74; c.set(29, 0x210a); back = 0x10288b2c; }
    } else if (compare(c, choice, 2) == 0) { effect = 0x10; c.set(29, 0x1ff9); back = 0x10288afc; }
    else if (choice > 2) { effect = 0xc; c.set(29, 0x1ff7); back = 0x10288ae4; }
    if (effect) {
        leave(c, {{3, actor.match()}, {4, player}}); c.set_f(1, c.f32(singles + effect)); c.call(0x101d6780, back);
    }
    if (compare(c, std::int16_t(c.r(29)), -1) > 0) {
        advance(0x10288b58); auto value = sample(); c.set(3, value >> 31);
        if (compare(c, std::int32_t(value) % 8) != 0 && compare(c, std::int8_t(c.r(25))) != 0) {
            advance(0x10288bb0); value = sample();
            leave(c, {{3, std::uint32_t(high_product(value, 0x55555556))}, {4, value}});
            if (compare(c, std::int32_t(value) % 3) != 0) {
                const auto opposing_organization = [&] {
                    const auto opposing = unsigned(actor.side() == 0); compare(c, opposing);
                    const auto scene = c.u32(actor.match() + 0x3118);
                    return c.u32(c.u32(scene + (opposing ? 0x20 : 0x1c)) + 0x2c);
                };
                if (compare_logical(c, opposing_organization()) != 0) {
                    const auto organization = opposing_organization();
                    if (compare(c, c.u8(organization + 0x119), 8) == 0) message(0x20f5, 0x10288c8c, 1);
                }
            }
            message(0x2109, 0x10288cb4);
        }
        message(c.r(29), 0x10288cdc);
        if (compare(c, c.u8(frame.at(0x38)), 3) == 0 || compare(c, c.u8(frame.at(0x38)), 4) == 0) {
            leave(c, {{3, actor.match()}, {4, player}, {5, 0}}); c.call(0x101f1530, 0x10288d04);
        }
    }
    c.set(3, actor.match()); c.set(4, c.u8(c.r(3) + 0x7924));
    c.set(3, unsigned(actor.side() == 0));
    if (compare(c, c.r(3), std::int8_t(c.r(4))) != 0) return;
    if (compare(c, c.i8(frame.at(0x38))) == 0 || compare(c, c.u8(frame.at(0x38)), 9) == 0) return;
    c.set(3, actor.match()); if (compare(c, c.i8(c.r(3) + 0x791e)) <= 0) return;
    c.set(3, actor.match()); if (compare(c, c.i8(c.r(3) + 0x791e), 11) >= 0) return;
    c.set(3, actor.match());
    if (compare(c, c.u8(c.r(3) + 0x7928), 6) != 0) {
        c.set(3, actor.match()); if (compare(c, c.u8(c.r(3) + 0x7928), 7) != 0) return;
    }
    c.set(3, c.u32(actor.match() + 0x3118));
    if (compare_logical(c, c.u32(c.r(3) + 0x14)) == 0) return;
    c.set(3, c.u32(c.u32(actor.match() + 0x3118) + 0x14));
    if (compare_logical(c, c.u32(c.r(3) + 8)) == 0) return;
    const auto scene = c.u32(actor.match() + 0x3118), clock = c.u32(c.toc(-0x7a10));
    leave(c, {{3, c.u32(c.u32(scene + 0x14) + 8)}, {4, clock}, {5, scene}});
    if (compare(c, clock, c.u32(c.r(3) + 0x68)) != 0) return;
    const bool second = compare(c, std::int8_t(actor.side())) != 0;
    c.set(4, actor.match()); c.set(3, c.u8(c.r(4) + 0x791e));
    c.put8(c.r(4) + 0x791e, c.r(3) + (second ? UINT32_MAX : 1));
    // The checked input row is1..10, so its single step lies in0..11;
    // both original saturation arms are unreachable.
    compare(c, c.i8(c.r(4) + 0x791e), 11); compare(c, c.i8(c.r(4) + 0x791e));
    message(0x21b9, 0x10288e9c, 2);
    const auto match = actor.match();
    leave(c, {{3, match}, {4, actor.side()}, {5, c.u8(match + 0x791d)}, {6, c.u8(match + 0x791e)}});
    c.call(0x101eea30, 0x10288eb8);
    if (compare(c, std::int8_t(c.r(3))) == 0) return;
    c.set(3, actor.match());
    if (compare(c, c.u8(c.r(3) + 0x7928), 6) == 0) message(0x21ba, 0x10288f08, 2);
    c.set(3, actor.match()); c.put8(c.r(3) + 0x7928, 7);
}
}
