#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;

std::uint32_t react_to_contact_failure(Cpu& c, std::uint32_t player, std::int16_t quality) {
    Frame frame(c, 0x10270320, 0x70, {.first_gpr = 24});
    const auto doubles = c.toc(-0x51e8), seed = c.toc(-0x7b90), begin = c.toc(-0x7b8c), end = c.toc(-0x7b88), cursor = c.toc(-0x7b84);
    keep(c, {{26, doubles}, {30, player}, {29, seed}, {28, begin}, {27, end}, {31, cursor}});
    const auto message = [&](unsigned id, std::uint32_t back) {
        const auto match = c.u32(player + 0x1a4);
        leave(c, {{3, c.u32(match + 0x79f4)}, {4, id}, {5, match}, {6, c.u8(player + 0x2f)},
                  {7, c.u8(player + 0x27)}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}});
        c.call(0x10201180, back);
    };
    if (compare(c, quality, 1000) > 0) { message(0x1fe2, 0x1027037c); return result(c, 1); }
    const auto advance = [&](std::uint32_t back, bool trait = false) {
        const auto next = c.u32(cursor) + 2; c.set(3, next);
        if (trait) keep(c, {{24, c.u8(player + 0x109)}});
        const auto ordering = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
        if (ordering > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
    };
    const auto sample = [&] { return std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed); };
    const auto shifted_sign = [&](std::int32_t high, unsigned bits) {
        c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << bits) - 1)));
        return std::uint32_t((high >> bits) < 0);
    };
    const auto magic_high = [](std::uint32_t value, std::int32_t multiplier) {
        return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * multiplier) >> 32);
    };
    const auto narrow = [&](std::int32_t bound) {
        c.set(3, sample()); compare(c, bound);
        const auto residual = std::int32_t(std::int64_t(std::int32_t(c.r(3))) % bound);
        const auto draw = bound < 0 ? -residual : residual;
        keep(c, {{25, std::uint32_t(draw)}}); return draw;
    };
    Fpu fp(c);
    c.set(3, c.u32(player + 0x70));
    c.put32(frame.at(0x48), 0x43300000);
    c.put32(frame.at(0x4c), std::uint32_t(std::int32_t(c.i8(c.r(3) + 0x21))) ^ 0x80000000);
    c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, c.f32(player + 0xa8));
    const auto converted = fp.fsubs(c.f64(frame.at(0x48)), c.f(1));
    c.put64(frame.at(0x40), fp.fctiwz(0, fp.fadds(c.f(2), converted)));
    const auto bound_word = c.u32(frame.at(0x44)); const auto bound = std::int32_t(bound_word);
    keep(c, {{24, bound_word}});
    std::int32_t draw = 0;
    if (compare(c, bound) == 0) keep(c, {{25, 0}});
    else {
        advance(0x102703ec);
        if (bound >= -65535 && bound <= 65535) draw = narrow(bound);
        else {
            c.put32(frame.at(0x44), bound_word ^ 0x80000000); c.put32(frame.at(0x40), 0x43300000);
            c.set_f(2, c.f64(doubles + 0x80)); c.set_f(1, fp.fsub(c.f64(frame.at(0x40)), c.f(2)));
            c.set_f(2, fp.fdiv(c.f(1), c.f64(doubles)));
            const auto word = c.u16(c.u32(cursor)); const auto value = std::uint32_t(word) + c.u32(seed);
            const auto residual = std::int32_t(value) % 65535;
            c.put32(frame.at(0x48), 0x43300000); c.put32(frame.at(0x4c), std::uint32_t(residual) ^ 0x80000000);
            c.set_f(1, c.f64(doubles + 0x80));
            const auto coarse = fp.fmul(c.f(2), fp.fsub(c.f64(frame.at(0x48)), c.f(1)));
            c.put64(frame.at(0x38), fp.fctiwz(0, coarse));
            const auto whole = c.u32(frame.at(0x3c)); keep(c, {{25, whole}});
            const auto reduced = std::int32_t(bound_word * 10u) / 65535;
            leave(c, {{3, std::uint32_t(reduced)}, {4, std::uint32_t(residual) ^ 0x80000000}, {5, 65535},
                      {6, std::uint32_t(std::int32_t(value) < 0)}, {7, value}, {8, word}});
            const auto scaled = std::int32_t(bound_word * 10u);
            const auto partial = std::int32_t(std::uint32_t(magic_high(std::uint32_t(scaled), std::int32_t(0x80008001))) + std::uint32_t(scaled));
            shifted_sign(partial, 15);
            enter(c, 0x102704c4, {std::uint32_t(reduced)}); sample_range(c, std::uint32_t(reduced));
            const auto numerator = std::int32_t(c.r(3) + 5), high = magic_high(std::uint32_t(numerator), 0x66666667);
            c.set(4, shifted_sign(high, 2)); c.set(3, std::uint32_t(numerator / 10));
            auto result_word = whole + c.r(3);
            if (compare(c, std::int32_t(result_word), std::int32_t(bound_word - 1)) > 0) result_word = bound_word - 1;
            keep(c, {{25, result_word}}); draw = std::int32_t(result_word);
        }
    }
    advance(0x10270548);
    auto value = sample(); auto high = magic_high(value, 0x66666667);
    leave(c, {{3, shifted_sign(high, 3)}, {4, value}});
    if (compare(c, draw, std::int32_t(value) % 20) >= 0) { message(0x1fe2, 0x10270b7c); return result(c, 1); }

    const auto request = [&](unsigned kind, bool active, bool self, std::uint32_t back) {
        leave(c, {{3, c.u32(player + 0x1a4)}, {4, c.u8(player + 0x105)}, {5, c.u8(player + 0x106)},
                  {6, kind}, {7, c.u8(player + 0x27)}, {8, unsigned(active)}, {9, 1}, {10, self ? player : 0}});
        c.call(0x101f1100, back);
    };
    const auto choose_request = [&](bool later) {
        advance(later ? 0x10270838 : 0x10270680);
        auto sum = sample(); auto product = magic_high(sum, 0x66666667);
        leave(c, {{3, shifted_sign(product, 2)}, {4, sum}});
        if (compare(c, std::int32_t(sum) % 10) != 0) {
            request(later ? 1 : 3, true, false, later ? 0x10270898 : 0x102706e0); return;
        }
        advance(later ? 0x102708bc : 0x10270704);
        sum = sample(); product = magic_high(sum, 0x51eb851f);
        leave(c, {{3, shifted_sign(product, 6)}, {4, sum}});
        const auto random = std::int32_t(sum) % 200;
        if (compare(c, random) != 0) request(later ? 1 : 3, false, false, later ? 0x1027091c : 0x10270764);
        else request(later ? 1 : 3, false, true, later ? 0x10270944 : 0x1027078c);
    };
    advance(0x102705ac, true);
    value = sample(); const auto sign = std::uint32_t(std::int32_t(value) < 0);
    leave(c, {{3, std::uint32_t(std::int32_t(value) % 8)}, {4, sign}});
    if (compare(c, std::int8_t(c.r(24)), std::int32_t(value) % 8) > 0) {
        message(0x1fe3, 0x1027060c); advance(0x1027062c);
        enter(c, 0x10270660, {player, std::uint32_t(std::int32_t(sample()) % 2)});
        add_pressure(c, player, c.r(4)); choose_request(false); return result(c, 0);
    }
    message(0x216f, 0x102707b8); advance(0x102707d8);
    value = sample(); high = magic_high(value, 0x55555556);
    leave(c, {{3, player}, {4, std::uint32_t(std::int32_t(value) % 3)}, {5, value}});
    enter(c, 0x10270818, {player, c.r(4)}); add_pressure(c, player, c.r(4));
    choose_request(true);
    c.set(3, std::uint32_t(std::int32_t(c.i16(player + 0x36)))); c.put16(player + 0x36, c.r(3) - 250);
    c.set(3, c.u32(player + 0x6c)); const auto recovery_bound = c.i8(c.r(3) + 0x66) * 30 + 20;
    c.set(3, std::uint32_t(recovery_bound - 20)); keep(c, {{24, std::uint32_t(recovery_bound)}});
    c.s.wr<1>(reg::xer_ca, std::uint32_t(recovery_bound - 20) > UINT32_MAX - 20); compare(c, recovery_bound);
    // 30*signed8+20 lies in [-3820,3830] and cannot equal 0.
    advance(0x10270990); draw = narrow(recovery_bound);
    if (compare(c, draw) != 0) return result(c, 0);
    advance(0x10270af4); value = sample(); high = magic_high(value, 0x55555556);
    leave(c, {{3, player}, {4, std::uint32_t(- (std::int32_t(value) % 3))}, {5, value}});
    enter(c, 0x10270b38, {player, c.r(4)}); adjust_morale(c, player, c.r(4));
    if (compare(c, std::int8_t(c.r(3))) != 0) { leave(c, {{3, player}, {4, 0}}); c.call(0x1022ba10, 0x10270b4c); }
    return result(c, 0);
}
}
