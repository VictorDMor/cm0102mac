#include "match_pl_ball_action_detail.hpp"

namespace cm::aot::match_pl::detail {
using namespace guest;

void BallAction::flight_phase() {
    if (compare(c, c.i16(at(0xee)), -1) > 0) player_message(unsigned(c.i16(at(0xee))), 0x10250620, true);
    if (compare(c, kind(), 52) == 0 || compare(c, kind(), 55) == 0) {
        c.set_f(0, c.f64(doubles + 8)); fp.fcmp(0, metric(), c.f(0));
        bool short_clearance = false;
        if (c.cr(0) & 8) {
            leave(c, {{3, match()}, {4, unsigned(std::int8_t(actor.side()) == 0)},
                      {5, c.r(26)}, {6, c.u8(at(0x187))}}); c.call(0x101eea30, 0x10250668);
            short_clearance = compare(c, std::int8_t(c.r(3))) == 0;
        }
        if (short_clearance) c.put8(match() + 0x791f, 0);
        else {
            advance(0x102506a0); const auto common = compare(c, modulo(40)) != 0;
            int phase = 4; if (!common) { advance(0x10250704); phase = modulo(3); }
            c.put8(match() + 0x791f, std::uint32_t(phase));
        }
        advance(0x10250760); const auto wait = 1 + modulo(3); c.set(5, match()); c.put8(c.r(5) + 0x7920, std::uint32_t(wait));
    } else if (compare(c, kind(), 30) == 0 || compare(c, kind(), 8) == 0) {
        c.put8(match() + 0x791f, 4); advance(0x102507e8); const auto wait = 1 + modulo(3);
        c.set(5, match()); c.put8(c.r(5) + 0x7920, std::uint32_t(wait));
    } else if (compare(c, kind(), 60) == 0 || compare(c, kind(), 31) == 0 || compare(c, kind(), 32) == 0) {
        advance(0x10250870); int reduction = 0;
        if (compare(c, modulo(3)) != 0) { advance(0x102508d0); reduction = binary(4); }
        c.s.wr<1>(reg::xer_ca, std::uint32_t(reduction) <= 4);
        c.put8(match() + 0x791f, std::uint32_t(4 - reduction));
        advance(0x10250928); c.put8(match() + 0x7920, std::uint32_t(1 + binary()));
    } else if (compare(c, kind(), 11) == 0 || compare(c, kind(), 7) == 0) {
        advance(0x10250998); const auto phase = unsigned(modulo(7) == 0); c.set(5, match()); c.put8(c.r(5) + 0x791f, phase);
        advance(0x10250a04); c.put8(match() + 0x7920, std::uint32_t(binary()));
    } else if (compare(c, kind(), 9) == 0 || compare(c, kind(), 62) == 0 || compare(c, kind(), 90) == 0) {
        advance(0x10250a7c); int reduction = 0;
        if (compare(c, modulo(35)) == 0) { advance(0x10250ae4); reduction = binary(); }
        c.s.wr<1>(reg::xer_ca, std::uint32_t(reduction) <= 4);
        c.put8(match() + 0x791f, std::uint32_t(4 - reduction));
        advance(0x10250b38); c.put8(match() + 0x7920, std::uint32_t(2 + binary()));
    } else {
        advance(0x10250b90);
        if (compare(c, modulo(3)) != 0) {
            advance(0x10250be8); const auto draw = binary(); c.s.wr<1>(reg::xer_ca, std::uint32_t(draw) <= 2);
            c.set(4, match()); c.put8(c.r(4) + 0x791f, std::uint32_t(2 - draw));
        } else {
            advance(0x10250c40); const auto draw = binary(4); c.s.wr<1>(reg::xer_ca, std::uint32_t(draw) <= 4);
            c.set(4, match()); c.put8(c.r(4) + 0x791f, std::uint32_t(4 - draw));
        }
        advance(0x10250c98); c.put8(match() + 0x7920, std::uint32_t(binary()));
    }

    // This phase snapshot survives reseeding and is not replaced by a later
    // match read. Negative signed remainders can increase it.
    c.set(3, c.u32(at(0x178))); c.set(4, match()); c.set(13, c.u8(c.r(4) + 0x791f));
    bool reduce = false;
    if (compare(c, c.i8(c.u32(at(0x178)))) != 0 && compare(c, std::int8_t(c.r(13))) > 0) {
        advance(0x10250d08); reduce = compare(c, modulo(3)) == 0;
    }
    if (!reduce && compare(c, std::int8_t(c.r(13)), 4) < 0) {
        advance(0x10250d6c); c.set(5, sample()); const auto word = c.r(5);
        const auto draw = std::int32_t(word) % 200; c.set(4, shifted_sign(high(word, 0x51eb851f), 6));
        c.set(3, std::uint32_t(draw)); reduce = compare(c, c.i16(player() + 0x1a0), draw) > 0;
    }
    if (reduce) {
        advance(0x10250dd0); const auto reduction = binary(); c.put8(match() + 0x791f, c.r(13) - std::uint32_t(reduction));
    }
}

std::uint32_t BallAction::publish() {
    const auto clamp = [&](int value, int limit) {
        const auto bounded = compare(c, value, limit) >= 0 ? limit : value;
        if (compare(c, bounded) <= 0) return 0;
        return compare(c, value, limit) >= 0 ? limit : value;
    };
    const auto target_row = clamp(to_row(), 11), target_column = clamp(to_column(), 8);
    leave(c, {{3, match()}, {4, c.u8(at(0x16f))}, {5, c.u8(at(0x173))},
              {6, std::uint32_t(target_column)}, {7, std::uint32_t(target_row)}}); c.call(0x101ee610, 0x10250e9c);
    c.set(4, match()); c.put16(c.r(4) + 0x7950, c.r(3));
    c.set(6, actor.side()); c.set(5, match()); const auto side = std::int8_t(c.r(6));
    const auto valid_side = compare(c, side, -1) != 0; c.put8(c.r(5) + 0x7921, c.r(6));
    if (valid_side) {
        c.set(4, c.r(5) + std::uint32_t(std::int32_t(side))); c.set(3, c.u8(c.r(4) + 0x77bc)); c.put8(c.r(4) + 0x77bc, c.r(3) + 1);
        c.set(3, c.r(5) + unsigned(side == 0));
        if (compare(c, c.i8(c.r(3) + 0x77bc)) > 0) {
            c.set(4, c.r(5) + unsigned(side == 0)); c.set(3, c.u8(c.r(4) + 0x77bc)); c.put8(c.r(4) + 0x77bc, c.r(3) - 10);
        }
    }
    if (compare_logical(c, receiver()) != 0) {
        bool mismatch = compare(c, to_column(), c.i8(receiver() + 0x105)) != 0;
        if (!mismatch) mismatch = compare(c, to_row(), c.i8(receiver() + 0x106)) != 0;
        if (mismatch) { c.set(3, c.u32(at(0x178))); c.put8(c.r(3), 1); }
    }
    const auto adjusted_score = compare(c, score(), -100) > 0 ? int(score()) : -100;
    leave(c, {{3, match()}, {4, c.u8(at(0x16f))}, {5, c.u8(at(0x173))}, {6, c.r(26)},
              {7, c.u8(at(0x187))}, {8, c.r(24)}, {9, std::uint32_t(adjusted_score)}, {10, 1}});
    c.call(0x101f0090, 0x10250f80);
    const auto outside = [&] {
        if (compare(c, c.i8(match() + 0x791d)) < 0) return true;
        if (compare(c, c.i8(match() + 0x791d), 9) >= 0) return true;
        if (compare(c, c.i8(match() + 0x791e)) < 0) return true;
        return compare(c, c.i8(match() + 0x791e), 12) >= 0;
    };
    if (compare(c, std::int8_t(c.r(3))) != 0) {
        if (compare(c, c.i8(match() + 0x7928)) != 0) return result(c, 0);
        if (outside()) {
            leave(c, {{3, match()}, {4, unsigned(std::int8_t(actor.side()) == 0)}}); c.call(0x101f0c00, 0x10251004);
        }
        return result(c, 0);
    }
    if (compare(c, to_column(), 4) == 0 && (compare(c, c.u8(at(0x187)), 12) == 0 || compare(c, to_row(), -1) == 0)) {
        const bool upper = compare(c, to_row(), 12) == 0;
        const bool first_side = compare(c, std::int8_t(actor.side())) == 0;
        c.set_f(1, metric()); c.set(3, player());
        if (upper) c.call(first_side ? 0x1026a1b0 : 0x1026d660, first_side ? 0x10251098 : 0x102510a8);
        else c.call(first_side ? 0x1026d660 : 0x1026a1b0, first_side ? 0x1025106c : 0x1025107c);
        if (compare(c, c.i8(match() + 0x7928)) != 0) return result(c, 0);
    }
    const auto denominator = [&] {
        c.set(3, match()); c.put32(at(0x94), 0x43300000);
        const auto wait = c.i8(c.r(3) + 0x7920); c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, c.f64(doubles + 0x2c0));
        c.put32(at(0x98), std::uint32_t(std::int32_t(wait)) ^ 0x80000000);
        c.set_f(0, c.f64(at(0x94))); c.set_f(0, fp.fsub(c.f(0), c.f(1))); c.set_f(0, fp.fadd(c.f(2), c.f(0)));
    };
    denominator(); c.set_f(3, c.f64(doubles + 0x10)); fp.fcmp(0, c.f(3), c.f(0));
    if (c.cr(0) & 4) c.set_f(0, c.f64(doubles + 0x10)); else denominator();
    c.set_f(1, c.f64(doubles + 0xc8)); c.set_f(0, fp.frsp(c.f(0))); c.set_f(2, c.f64(doubles + 0x28));
    c.set_f(1, fp.fmul(c.f(1), metric())); c.set(3, match()); c.set_f(0, fp.fdiv(c.f(1), c.f(0)));
    c.set_f(0, fp.fadd(c.f(2), c.f(0))); c.set_f(0, fp.frsp(c.f(0))); c.put64(at(0x94), fp.fctiwz(0, c.f(0)));
    c.put8(c.r(3) + 0x1de, c.u32(at(0x98)));
    const auto row = c.u8(at(0x187)); c.put8(match() + 0x1df, c.r(26)); c.put8(match() + 0x1e0, row);
    c.put8(match() + 0x1e1, c.u8(player() + 0x105)); c.put8(match() + 0x1e2, c.u8(player() + 0x106));
    const auto ball_row = c.u8(at(0x187)); c.put32(match() + 0x3104, receiver());
    c.put8(match() + 0x791d, c.r(26)); c.put8(match() + 0x791e, ball_row);
    if (outside()) { leave(c, {{3, match()}, {4, actor.side()}}); c.call(0x101f0c00, 0x10251200); }
    return result(c, unsigned(compare(c, c.i8(match() + 0x7928)) == 0));
}
}
