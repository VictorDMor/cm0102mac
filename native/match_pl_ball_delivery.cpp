#include "match_pl_ball_action_detail.hpp"

namespace cm::aot::match_pl::detail {
using namespace guest;

void BallAction::delivery() {
    const auto morale = [&](std::uint32_t random_back, std::uint32_t correction,
                            std::uint32_t change_back, std::uint32_t response_back, std::uint32_t random_change_back) {
        const auto trait = c.i8(c.u32(player() + 0x6c) + 0x66);
        const auto bound = trait * 20 + 10; c.set(15, std::uint32_t(bound));
        c.s.wr<1>(reg::xer_ca, std::uint32_t(trait * 20) > UINT32_MAX - 10u);
        range(bound, 13, random_back, correction, true);
        if (compare(c, std::int32_t(c.r(13))) != 0) return;
        advance(random_change_back); c.set(5, sample()); const auto change = std::int8_t(-modulo(3));
        enter(c, change_back, {player(), std::uint32_t(std::int32_t(change))}); adjust_morale(c, player(), std::uint32_t(std::int32_t(change)));
        if (compare(c, std::int8_t(c.r(3))) != 0) {
            leave(c, {{3, player()}, {4, 0}}); c.call(0x1022ba10, response_back);
        }
    };
    if (!opposite_area(player())) {
        c.set(15, std::uint32_t(std::int32_t(c.i16(player() + 0x32)))); range(std::int32_t(c.r(15)), 13, 0x1024b35c, 0x1024b434, true);
        if (compare(c, std::int32_t(c.r(13)), -10) < 0) {
            c.set_f(1, c.f32(singles + 0x18)); enter(c, 0x1024b4ac, {player()}); scale_intensity(c, player(), c.f(1));
            side_message(0x211d, unsigned(std::int8_t(actor.side()) == 0), 0x1024b4e4);
            morale(0x1024b524, 0x1024b5fc, 0x1024b6cc, 0x1024b6e0, 0x1024b688);
        }
    }
    c.put32(match() + 0xdff4, player()); c.put16(match() + 0x77c0, std::uint32_t(std::int32_t(kind())));
    c.set(3, c.u8(player() + 1)); c.put8(player() + 1, c.r(3) + 1);
    morale(0x1024b744, 0x1024b81c, 0x1024b8ec, 0x1024b900, 0x1024b8a8);
    if (compare_logical(c, c.u32(match() + 0xe000)) != 0) {
        c.set(7, match()); c.set(14, c.u8(match() + 0x1c0)); c.set(4, UINT32_MAX); c.set(6, 0); c.set(5, 0); c.set(3, 0);
        c.put32(c.r(7) + 0x310c, 0); c.set(7, match()); c.put8(c.r(7) + 0x7924, c.r(4));
        c.set(4, match() + 0x10000); c.put32(c.r(4) - 0x2000, c.r(6)); c.set(4, match()); c.put8(c.r(4) + 0x1c0, c.r(5));
        c.put16(player() + 0x2a, c.r(3)); c.put16(player() + 0x1a0, 0);
    }
    c.set(4, match()); c.put32(at(0x94), 0x43300000); c.put16(c.r(4) + 0x794e, c.r(31) - std::uint32_t(c.i16(at(0x4a))));
    c.put8(c.r(4) + 0x792f, c.u8(at(0x16f))); c.put8(c.r(4) + 0x791c, c.u8(at(0x173)));
    c.set_f(2, c.f64(doubles + 0x80)); c.set_f(3, c.f64(doubles + 0x20)); c.set_f(0, c.f64(doubles + 0x10));
    c.set(4, c.u8(match() + 0x791f)); c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(4)))) ^ 0x80000000);
    c.put32(at(0x98), c.r(3));
    const auto weather = c.i8(match() + 0x30d8); c.set_f(1, fp.fsub(c.f64(at(0x94)), c.f(2)));
    c.set_f(30, fp.multiply_add(0x1024b9c0, c.f(3), c.f(1), c.f(0))); c.set_f(30, fp.frsp(c.f(30)));
    unsigned coefficient = 0;
    switch (weather) { case 1: coefficient = 0x9c; break; case 2: coefficient = 0x98; break;
    case 3: coefficient = 0xfc; break; case 4: coefficient = 0x100; break; default: break; }
    if (coefficient) { c.set_f(0, c.f32(singles + coefficient)); c.set_f(30, fp.fmuls(c.f(30), c.f(0))); }
}

void BallAction::accuracy() {
    const auto bound = [&](unsigned destination) {
        c.set_f(destination, c.f32(singles + 0x5c)); c.set_f(destination, fp.fmuls(c.f(destination), c.f(30)));
        c.set_f(destination, fp.fmuls(c.f(destination), c.f(30)));
    };
    bound(1); c.set_f(0, c.f32(singles)); fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 2) c.set(13, 0);
    else {
        advance(0x1024ba58); bound(0); c.set(3, 65535); c.put64(at(0x94), fp.fctiwz(0, c.f(0)));
        bool narrow = compare(c, c.i32(at(0x98)), 65535) <= 0;
        if (narrow) {
            bound(0); c.set(3, std::uint32_t(-65535)); c.put64(at(0x94), fp.fctiwz(0, c.f(0)));
            narrow = compare(c, c.i32(at(0x98)), -65535) >= 0;
        }
        if (narrow) {
            c.set_f(0, c.f32(singles + 0x5c)); c.set(3, c.u32(cursor)); c.set_f(1, fp.fmuls(c.f(0), c.f(30)));
            c.set_f(0, c.f32(singles + 0x5c)); const auto word = std::uint32_t(c.u16(c.r(3))) + c.u32(seed); c.set(5, word);
            c.set_f(0, fp.fmuls(c.f(0), c.f(30))); c.set_f(1, fp.fmuls(c.f(1), c.f(30))); c.set_f(0, fp.fmuls(c.f(0), c.f(30)));
            c.put64(at(0x94), fp.fctiwz(1, c.f(1))); c.set(4, c.u32(at(0x98))); c.put64(at(0x9c), fp.fctiwz(0, c.f(0)));
            const auto q = c.r(4) ? std::uint32_t(divide_signed<4>(word, c.r(4))) : 0u; c.set(3, q);
            const bool positive = compare(c, c.i32(at(0xa0))) > 0; const auto residual = word - q * c.r(4);
            c.set(13, positive ? residual : 0u - residual);
        } else {
            bound(1); bound(0); c.set(7, c.u32(cursor)); c.put32(at(0x9c), 0x43300000); c.put32(at(0xa4), 0x43300000);
            c.set_f(3, c.f64(doubles + 0x80)); c.set_f(2, c.f64(doubles));
            c.put64(at(0x94), fp.fctiwz(1, c.f(1))); c.put64(at(0xb4), fp.fctiwz(0, c.f(0)));
            c.set(7, sample()); c.set_f(1, c.f64(doubles + 0x80)); c.put32(at(0xa0), c.u32(at(0x98)) ^ 0x80000000);
            const auto word = c.r(7); const auto product = std::int32_t(std::uint32_t(high(word, std::int32_t(0x80008001))) + word);
            c.set(6, shifted_sign(product, 15)); c.set_f(0, fp.fsub(c.f64(at(0x9c)), c.f(3))); c.set_f(2, fp.fdiv(c.f(0), c.f(2)));
            c.set(13, word - std::uint32_t(std::int32_t(word) / 65535) * 65535u); c.set(4, c.r(13) ^ 0x80000000);
            c.set(5, 65535); c.put32(at(0xa8), c.r(4));
            const auto numerator = c.u32(at(0xb8)) * 10; c.set_f(0, fp.fsub(c.f64(at(0xa4)), c.f(1))); c.set_f(0, fp.fmul(c.f(2), c.f(0)));
            shifted_sign(std::int32_t(std::uint32_t(high(numerator, std::int32_t(0x80008001))) + numerator), 15);
            c.set(3, std::uint32_t(std::int32_t(numerator) / 65535)); c.put64(at(0xac), fp.fctiwz(0, c.f(0))); c.set(13, c.u32(at(0xb0)));
            enter(c, 0x1024bb90, {c.r(3)}); sample_range(c, c.r(3));
            const auto adjustment = c.r(3) + 5; bound(0); shifted_sign(high(adjustment, 0x66666667), 2);
            c.put64(at(0xbc), fp.fctiwz(0, c.f(0))); c.set(13, c.r(13) + std::uint32_t(std::int32_t(adjustment) / 10)); c.set(3, c.u32(at(0xc0)));
            if (compare(c, std::int32_t(c.r(13)), std::int32_t(c.r(3) - 1)) > 0) {
                bound(0); c.put64(at(0xc4), fp.fctiwz(0, c.f(0))); c.set(3, c.u32(at(0xc8))); c.set(13, c.r(3) - 1);
            }
        }
    }
    c.set(3, std::uint32_t(std::int32_t(c.i16(at(0x4a)))));
    if (compare(c, int(score()) - std::int32_t(c.r(3)), std::int32_t(c.r(13))) >= 0) return;
    c.set_f(1, fp.fmuls(c.f(30), metric())); c.set_f(2, c.f64(doubles + 0x10)); c.set_f(0, c.f64(doubles + 0x28));
    c.set_f(1, fp.fadd(c.f(2), c.f(1))); c.set_f(0, fp.fmul(c.f(1), c.f(0))); c.put64(at(0x94), fp.fctiwz(0, c.f(0))); c.set(30, c.u32(at(0x98)));
    range(std::int8_t(c.r(30)), 16, 0x1024bcb8, 0x1024bda0, true, true); range(std::int8_t(c.r(30)), 15, 0x1024be40, 0x1024bf28, true, true);
    c.set(13, std::uint32_t(std::int32_t(std::int8_t(c.r(26) + c.r(16) - c.r(15)))));
    enter(c, 0x1024bfac, {player()}); current_role(c, player());
    bool keep_row = false;
    if (compare(c, std::int16_t(c.r(3)), 1) == 0) {
        c.set(16, std::uint32_t(c.i16(player() + 0x2a) * 50 + 20000));
        range(std::int32_t(c.r(16)), 15, 0x1024bff0, 0x1024c0c8); keep_row = compare(c, std::int32_t(c.r(15))) != 0;
    }
    if (keep_row) c.set(15, std::uint32_t(std::int32_t(to_row())));
    else {
        range(std::int8_t(c.r(30)), 15, 0x1024c164, 0x1024c24c, true, true); range(std::int8_t(c.r(30)), 16, 0x1024c2ec, 0x1024c3d4, true, true);
        c.set(15, std::uint32_t(std::int32_t(std::int8_t(c.u8(at(0x187)) + c.r(15) - c.r(16)))));
    }
    if (compare(c, std::int8_t(c.r(13)), to_column()) == 0 && compare(c, std::int8_t(c.r(15)), to_row()) == 0) return;
    if (compare_logical(c, receiver()) != 0) {
        enter(c, 0x1024c494, {player()}); current_role(c, player());
        if (compare(c, std::int16_t(c.r(3)), 1) == 0) condition_delta(player(), -50);
        else {
            enter(c, 0x1024c4bc, {player(), c.u8(at(0x173))}); row_in_own_half(c, player(), from_row());
            if (compare(c, std::int8_t(c.r(3))) != 0) {
                enter(c, 0x1024c4d0, {player(), c.r(15)}); row_in_own_half(c, player(), std::int8_t(c.r(15)));
                if (compare(c, std::int8_t(c.r(3))) != 0) condition_delta(player(), -110);
            }
        }
        bool crossing = compare_logical(c, std::uint8_t(c.r(24) - 7), 1) <= 0;
        if (!crossing) crossing = compare(c, kind(), 94) == 0;
        if (!crossing) crossing = compare(c, kind(), 6) == 0;
        condition_delta(player(), crossing ? -15 : -100);
    }
    c.put8(at(0x187), c.r(15)); c.set(26, c.r(13));
    leave(c, {{3, match()}, {4, c.u8(at(0x16f))}, {5, c.u8(at(0x173))}, {6, c.r(26)}, {7, c.u8(at(0x187))}});
    c.call(0x101ef8d0, 0x1024c548); c.set_f(31, c.f(1));
}

void BallAction::exertion() {
    if (compare_logical(c, receiver()) == 0) { condition_delta(player(), -36); return; }
    if (compare(c, c.i8(c.u32(at(0x178)))) != 0) {
        leave(c, {{3, match()}, {4, actor.side()}, {5, c.u8(at(0x187))}}); c.call(0x101ee9f0, 0x1024c574);
        if (compare(c, std::int8_t(c.r(3))) != 0) {
            enter(c, 0x1024c584, {player()}); current_role(c, player());
            condition_delta(player(), compare(c, std::int16_t(c.r(3)), 1) == 0 ? -13 : -20); return;
        }
    }
    enter(c, 0x1024c5b8, {player()}); current_role(c, player());
    if (compare(c, std::int16_t(c.r(3)), 1) == 0) condition_delta(player(), -66);
    else {
        enter(c, 0x1024c5e0, {player(), c.u8(at(0x173))}); row_in_own_half(c, player(), from_row());
        if (compare(c, std::int8_t(c.r(3))) != 0) {
            enter(c, 0x1024c5f4, {player(), c.u8(at(0x187))}); row_in_own_half(c, player(), to_row());
            if (compare(c, std::int8_t(c.r(3))) != 0) condition_delta(player(), -100);
        }
    }
    bool crossing = compare_logical(c, std::uint8_t(c.r(24) - 7), 1) <= 0;
    if (!crossing) crossing = compare(c, kind(), 6) == 0;
    condition_delta(player(), crossing ? -20 : -72);
}
}
