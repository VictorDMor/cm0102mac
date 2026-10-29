#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;

// Keep snapshots across callbacks; reload later actor and match fields live.
void choose_context_action(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x102836a0, 0x100, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 30});
    const auto messages = c.toc(-0x7ba8), distances = c.toc(-0x6424), doubles = c.toc(-0x51e8),
               seed = c.toc(-0x7b90), begin = c.toc(-0x7b8c), end = c.toc(-0x7b88),
               cursor = c.toc(-0x7b84), strings = c.toc(-0x51e4), singles = c.toc(-0x51e0);
    keep(c, {{13, messages}, {14, distances}, {15, strings}, {16, 0}, {18, 0}, {20, 0}, {23, end},
             {24, singles}, {25, begin}, {26, doubles}, {29, seed}, {30, cursor}, {31, player}});
    const Player actor{c, player}; Fpu fp(c);
    const auto high_product = [](std::uint32_t value, std::int32_t multiplier) {
        return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * multiplier) >> 32);
    };
    const auto shifted_sign = [&](std::int32_t high, unsigned bits) {
        c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << bits) - 1)));
        return std::uint32_t(high < 0);
    };
    const auto advance_with = [&](std::uint32_t back, auto snapshots) {
        const auto next = c.u32(cursor) + 2; c.set(3, next); snapshots();
        const auto order = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
        if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
    };
    const auto advance = [&](std::uint32_t back) { advance_with(back, [] {}); };
    const auto sample = [&] { return std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed); };
    const auto role_has = [&](std::uint32_t back, std::uint32_t mask) {
        enter(c, back, {player}); current_role(c, player); return compare(c, c.r(3) & mask) != 0;
    };
    const auto move = [&](std::uint32_t column, std::uint32_t row, std::uint32_t kind, std::uint32_t back) {
        leave(c, {{3, player}, {4, column}, {5, row}, {6, kind}, {7, 0}, {8, 0}}); c.call(0x1022e7e0, back);
    };
    const auto move_to_ball = [&](std::uint32_t back) {
        const auto match = actor.match(); move(c.u8(match + 0x791d), c.u8(match + 0x791e), 0x100, back);
        c.put16(player + 0x19c, 0x100);
    };
    const auto prepare_target = [&](std::uint32_t back, std::uint32_t bias_back) {
        leave(c, {{3, actor.match()}, {4, player}, {5, frame.at(0x3b)}, {6, frame.at(0x3a)}, {7, 1}}); c.call(0x101e28b0, back);
        enter(c, bias_back, {player, c.u8(frame.at(0x3b))}); adjust_action_bias(c, player, c.r(4)); c.put8(frame.at(0x3b), c.r(3));
    };
    const auto own_goal_area = [&] {
        const bool second = compare(c, actor.side(), 1) == 0; c.set(3, 0);
        bool inside = compare(c, actor.column(), 2) >= 0 && compare(c, actor.column(), 6) <= 0;
        if (inside) inside = second ? compare(c, actor.row(), 10) >= 0 && compare(c, actor.row(), 11) <= 0 :
                                     compare(c, actor.row()) >= 0 && compare(c, actor.row(), 1) <= 0;
        c.set(3, inside); compare(c, inside); return inside;
    };
    const auto pursuit_half = [&] {
        bool selected = compare(c, actor.side(), 1) == 0 && compare(c, actor.row(), 6) >= 0;
        if (!selected) selected = compare(c, std::int8_t(actor.side())) == 0 && compare(c, actor.row(), 6) < 0;
        compare(c, selected); return selected;
    };
    const auto publish = [&](std::uint32_t event, std::uint32_t back) {
        const auto match = actor.match();
        leave(c, {{3, c.u32(match + 0x79f4)}, {4, event}, {5, match}, {6, c.u8(player + 0x2f)},
                  {7, actor.side()}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}}); c.call(0x10201180, back);
    };
    const auto reset_heading = [&](std::uint32_t back) {
        enter(c, back, {player, c.u8(player + 0x105), c.u8(player + 0x106), 4, actor.side() == 0 ? 11u : 0u});
        set_heading(c, player, c.r(4), c.r(5), c.r(6), c.r(7));
    };

    c.set(3, actor.match());
    if (compare(c, c.i8(c.r(3) + 0x7924), -1) == 0) {
        c.set(3, actor.match() + 0x10000);
        if (compare_logical(c, c.u32(c.r(3) - 0x2008), player) == 0) {
            advance_with(0x10283738, [&] { c.set(17, c.u8(player + 0x10a)); });
            const auto value = sample(); const auto sign = shifted_sign(high_product(value, 0x66666667), 3);
            leave(c, {{3, std::uint32_t(std::int32_t(value) % 20)}, {4, sign}, {5, value}});
            if (compare(c, std::int8_t(c.r(17)) + std::int32_t(c.r(3))) != 0) { move_to_ball(0x102837a0); return; }
        }
        c.set(3, actor.match() + 0x10000);
        if (compare_logical(c, c.u32(c.r(3) - 0x2008)) == 0) {
            leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101f5280, 0x102837cc);
            if (compare(c, std::int8_t(c.r(3))) != 0) {
                c.set(3, actor.match());
                if (compare(c, c.i8(c.r(3) + 0x1de)) == 0) { move_to_ball(0x10283808); return; }
            }
        }
    }
    const auto side = std::uint32_t(std::int32_t(std::int8_t(actor.side()))), match = actor.match();
    leave(c, {{3, match + side * 0x18e0 + 0x7c6c}, {4, c.u8(player + 0x19)}, {5, match}}); c.call(0x101149f0, 0x10283834);
    if (compare(c, c.r(3) & 4) != 0) c.set(18, 1);
    c.put16(player + 0x19c, UINT32_MAX); c.set(27, c.u8(actor.match() + 0x791d)); c.set(28, c.u8(actor.match() + 0x791e));
    leave(c, {{3, actor.match()}, {4, player}, {5, actor.match()}}); c.call(0x101e3d50, 0x10283864);
    c.set(22, c.r(3)); c.set(4, actor.match());
    if (compare(c, c.i8(c.r(4) + 0x791f), -1) == 0) {
        c.set(3, actor.match());
        if (compare(c, c.i8(c.r(3) + 0x7924), -1) != 0) {
            leave(c, {{3, actor.match()}, {4, player}, {5, frame.at(0x39)}, {6, frame.at(0x38)}}); c.call(0x101e25a0, 0x102838a4);
            c.put16(player + 0x19c, 0x68); enter(c, 0x102838bc, {player, c.u8(frame.at(0x39)), c.u8(frame.at(0x38))});
            set_coordinates(c, player, c.r(4), c.r(5)); return;
        }
    }
    const bool second_side = compare(c, std::int8_t(actor.side())) != 0;
    advance_with(second_side ? 0x10283ba0 : 0x102838f8, [&] {
        keep(c, {{17, c.u8(player + 0x108)}, {19, c.u8(player + 0x10d)}, {21, c.u8(player + 0x109)}});
    });
    const auto first_value = sample(); const auto first_sign = shifted_sign(high_product(first_value, 0x51eb851f), 3);
    leave(c, {{3, std::uint32_t(std::int32_t(first_value) % 25)}, {4, std::uint32_t(std::int32_t(std::int8_t(c.r(17))))},
              {5, first_sign}, {6, first_value}, {7, first_value - c.u32(seed)}});
    const auto traits = std::int8_t(c.r(17)) * std::int8_t(c.r(19)) * std::int8_t(c.r(21));
    if (compare(c, traits, std::int32_t(c.r(3))) > 0) {
        c.set(3, actor.match() + 0x10000);
        if (compare_logical(c, c.u32(c.r(3) - 0x2000)) != 0 && compare(c, std::int16_t(c.r(22)), 1) != 0) {
            c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(28)))));
            const auto order = compare(c, std::int32_t(c.r(3)), actor.row());
            if ((!second_side && order > 0 && compare(c, actor.row(), 6) < 0) ||
                (second_side && order < 0 && compare(c, actor.row(), 5) > 0)) {
                const auto line_field = second_side ? 0x7925u : 0x7926u;
                const auto peer_field = second_side ? 0xe014u : 0xe018u;
                const auto limit = [&] {
                    c.set(5, actor.match()); c.set(4, actor.match()); c.set(3, c.r(5) + 0x10000);
                    c.set(4, c.u8(c.r(4) + line_field)); const auto peer = c.u32(c.r(5) + peer_field);
                    const bool exists = compare_logical(c, peer) != 0;
                    c.set(3, exists ? std::uint32_t(std::int32_t(c.i8(peer + 0x106))) : second_side ? UINT32_MAX : 9u);
                    return std::int32_t(c.r(3));
                };
                const auto initial_limit = limit(); const auto order = compare(c, initial_limit, std::int8_t(c.r(4)));
                if (second_side ? order >= 0 : order <= 0) {
                    c.set(3, actor.match() + 0x10000 + std::uint32_t(std::int32_t(std::int8_t(actor.side()))) * 0x18e0);
                    if (compare(c, c.u32(c.r(3) - 0x7e0c) & 0x400) != 0) {
                        const auto delta = [&] {
                            if (second_side) { c.set(4, c.u8(actor.match() + line_field)); c.set(3, std::uint32_t(std::int32_t(actor.row()))); return std::int8_t(c.r(4)) - actor.row(); }
                            c.set(3, std::uint32_t(std::int32_t(c.i8(actor.match() + line_field)))); return actor.row() - std::int32_t(c.r(3));
                        };
                        if (compare(c, delta(), 2) < 0 && compare(c, delta(), -2) > 0) {
                            c.put16(player + 0x19c, 0x7a);
                            const auto row = c.u8(actor.match() + line_field) + (second_side ? -2u : 2u);
                            move(c.u8(player + 0x105), std::uint32_t(std::int32_t(std::int8_t(row))), UINT32_MAX, second_side ? 0x10283d0c : 0x10283a64); return;
                        }
                    }
                    advance_with(second_side ? 0x10283d38 : 0x10283a90, [&] { c.set(17, c.u8(player + 0x10e)); c.set(19, c.u8(player + 0x109)); });
                    const auto value = sample(); const auto sign = shifted_sign(high_product(value, 0x51eb851f), 5);
                    leave(c, {{3, std::uint32_t(std::int32_t(std::int8_t(c.r(17))))}, {4, std::uint32_t(std::int32_t(value) % 100)}, {5, sign}, {6, value}});
                    if (compare(c, std::int8_t(c.r(17)) * std::int8_t(c.r(19)), std::int32_t(c.r(4))) > 0) {
                        if (second_side) c.set(3, std::uint32_t(std::int32_t(actor.row())));
                        else c.set(3, std::uint32_t(std::int32_t(c.i8(actor.match() + line_field))));
                        c.set(4, second_side ? c.u8(actor.match() + line_field) : c.r(4));
                        const auto delta = second_side ? std::int8_t(c.r(4)) - actor.row() : actor.row() - std::int32_t(c.r(3));
                        if (compare(c, delta, 1) == 0) {
                            const auto peer_limit = limit() + (second_side ? -1 : 1); c.set(3, std::uint32_t(peer_limit));
                            const auto order = compare(c, peer_limit, std::int8_t(c.r(4)));
                            if (second_side ? order >= 0 : order <= 0) {
                                c.put16(player + 0x19c, 0x7a);
                                const auto row = c.u8(actor.match() + line_field) + (second_side ? -2u : 2u);
                                move(c.u8(player + 0x105), std::uint32_t(std::int32_t(std::int8_t(row))), UINT32_MAX, second_side ? 0x10283e18 : 0x10283b70); return;
                            }
                        }
                    }
                }
            }
        }
    }
    c.set(3, c.u8(player + 0x105)); compare(c, std::int16_t(c.r(22)), 1);
    c.put8(frame.at(0x39), c.r(3)); c.put8(frame.at(0x38), c.u8(player + 0x106));
    bool may_chase = std::int16_t(c.r(22)) != 1;
    if (may_chase) {
        c.set(3, actor.match());
        if (compare(c, c.i8(c.r(3) + 0x7924), -1) != 0) {
            c.set(3, actor.match() + 0x10000);
            may_chase = compare_logical(c, c.u32(c.r(3) - 0x2000)) != 0;
            if (may_chase) {
                c.set(4, c.u32(actor.match() + 0xe000)); c.set(3, std::uint32_t(std::int32_t(std::int8_t(actor.side()))));
                may_chase = compare(c, std::int32_t(c.r(3)), c.i8(c.r(4) + 0x27)) != 0;
                if (may_chase) {
                    const auto main = c.u32(actor.match() + 0xe000);
                    leave(c, {{3, actor.match()}, {4, unsigned(std::int8_t(actor.side()) == 0)}, {5, c.u8(main + 0x106)}}); c.call(0x101e4ea0, 0x10283eb0);
                    may_chase = compare(c, std::int8_t(c.r(3))) >= 0;
                }
            }
        }
        if (may_chase) {
            c.set(3, actor.match() + 0x10000);
            if (compare_logical(c, c.u32(c.r(3) - 0x2008)) != 0) {
                c.set(4, c.u32(actor.match() + 0xdff8)); c.set(3, std::uint32_t(std::int32_t(std::int8_t(actor.side()))));
                may_chase = compare(c, std::int32_t(c.r(3)), c.i8(c.r(4) + 0x27)) != 0;
            }
        }
        if (may_chase) {
            leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101f5280, 0x10283efc);
            if (compare(c, std::int8_t(c.r(3))) != 0) { move_to_ball(0x10283f28); return; }
            may_chase = role_has(0x10283f3c, 0x200) || role_has(0x10283f4c, 0x100) || role_has(0x10283f5c, 0x400);
            if (may_chase) may_chase = role_has(0x10283f6c, 8) || role_has(0x10283f7c, 0x10) || role_has(0x10283f8c, 0x20);
        }
        if (may_chase) {
            c.set(3, actor.match()); c.set(4, actor.match());
            const auto ball_column = c.i8(c.r(3) + 0x791d); const auto ball_row = c.u8(c.r(4) + 0x791e);
            c.set(17, ball_row); c.set(21, c.u8(player + 0x110));
            c.set(3, std::uint32_t(std::int32_t(actor.column()) - ball_column)); c.call(0x10775540, 0x10283fbc);
            c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(17))))); c.set(17, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
            c.set(3, std::uint32_t(std::int32_t(actor.row()) - std::int8_t(c.r(4)))); c.call(0x10775540, 0x10283fd4);
            c.put64(frame.at(0x94), fp.fctiwz(0, c.f32(player + 0xe0)));
            c.set(3, std::uint32_t(std::int8_t(c.r(3)) * 36));
            c.set_f(31, c.f32(distances + c.r(3) + (std::uint32_t(std::int8_t(c.r(17))) << 2)));
            c.set(17, c.u32(frame.at(0x98)));
            const auto bound = std::int32_t(c.r(17));
            if (compare(c, bound) == 0) c.set(19, 0);
            else {
                advance(0x1028402c); c.set(3, 65535); bool narrow = compare(c, bound, 65535) <= 0;
                if (narrow) { c.set(3, 0xffff0001); narrow = compare(c, bound, -65535) >= 0; }
                if (narrow) {
                    c.set(3, sample()); compare(c, bound);
                    const auto residual = std::int32_t(std::int64_t(std::int32_t(c.r(3))) % bound);
                    c.set(19, bound > 0 ? std::uint32_t(residual) : 0u - std::uint32_t(residual));
                } else {
                    c.put32(frame.at(0x94), 0x43300000); c.put32(frame.at(0x98), c.r(17) ^ 0x80000000);
                    c.set_f(2, c.f64(doubles + 0x80)); c.set_f(1, fp.fsub(c.f64(frame.at(0x94)), c.f(2)));
                    c.set_f(2, fp.fdiv(c.f(1), c.f64(doubles)));
                    const auto word = c.u16(c.u32(cursor)); const auto value = std::uint32_t(word) + c.u32(seed);
                    const auto quotient = std::int32_t(value) / 65535, residual = std::int32_t(value) % 65535;
                    const auto sign = shifted_sign(std::int32_t(std::uint32_t(high_product(value, std::int32_t(0x80008001))) + value), 15);
                    c.put32(frame.at(0x8c), 0x43300000); c.put32(frame.at(0x90), std::uint32_t(residual) ^ 0x80000000);
                    c.set_f(1, c.f64(doubles + 0x80));
                    c.put64(frame.at(0x84), fp.fctiwz(0, fp.fmul(c.f(2), fp.fsub(c.f64(frame.at(0x8c)), c.f(1)))));
                    c.set(19, c.u32(frame.at(0x88)));
                    const auto multiplied = c.r(17) * 10u, reduced = std::uint32_t(std::int32_t(multiplied) / 65535);
                    shifted_sign(std::int32_t(std::uint32_t(high_product(multiplied, std::int32_t(0x80008001))) + multiplied), 15);
                    leave(c, {{3, reduced}, {4, 0x80008001}, {5, std::uint32_t(quotient) * 65535u}, {6, std::uint32_t(quotient)}, {7, sign}, {8, word}});
                    enter(c, 0x10284104, {reduced}); sample_range(c, reduced);
                    const auto numerator = c.r(3) + 5;
                    c.set(4, shifted_sign(high_product(numerator, 0x66666667), 2)); c.set(3, std::uint32_t(std::int32_t(numerator) / 10)); c.set(19, c.r(19) + c.r(3));
                    if (compare(c, std::int32_t(c.r(19)), std::int32_t(c.r(17) - 1)) > 0) c.set(19, c.r(17) - 1);
                }
            }
            c.set_f(1, fp.fmuls(c.f32(singles + 0xc8), c.f(31))); c.set_f(0, c.f32(singles)); fp.fcmp(0, c.f(1), c.f(0));
            if (c.cr(0) & 2) c.set(17, 0);
            else {
                advance(0x102841a4);
                c.put64(frame.at(0x84), fp.fctiwz(0, fp.fmuls(c.f32(singles + 0xc8), c.f(31))));
                c.set(3, 65535); bool narrow = compare(c, c.i32(frame.at(0x88)), 65535) <= 0;
                if (narrow) {
                    c.put64(frame.at(0x84), fp.fctiwz(0, fp.fmuls(c.f32(singles + 0xc8), c.f(31))));
                    c.set(3, 0xffff0001); narrow = compare(c, c.i32(frame.at(0x88)), -65535) >= 0;
                }
                if (narrow) {
                    c.set_f(1, fp.fmuls(c.f32(singles + 0xc8), c.f(31))); c.set_f(0, fp.fmuls(c.f32(singles + 0xc8), c.f(31)));
                    const auto value = sample(); c.put64(frame.at(0x64), fp.fctiwz(1, c.f(1))); c.put64(frame.at(0x6c), fp.fctiwz(0, c.f(0)));
                    const auto divisor = c.u32(frame.at(0x68));
                    // A nonzero float can truncate to zero, or the reseed can
                    // change the coefficient. This original divw has a proven
                    // nontrapping zero-divisor path: quotient0, then negated
                    // dividend as the oriented remainder.
                    const auto quotient = divisor == 0 ? 0u : std::uint32_t(divide_signed<4>(value, divisor));
                    leave(c, {{3, quotient}, {4, divisor}, {5, value}}); compare(c, c.i32(frame.at(0x70)));
                    const auto residual = value - quotient * divisor;
                    c.set(17, c.i32(frame.at(0x70)) > 0 ? residual : 0u - residual);
                } else {
                    c.put32(frame.at(0x8c), 0x43300000);
                    c.set_f(1, fp.fmuls(c.f32(singles + 0xc8), c.f(31))); c.set_f(0, fp.fmuls(c.f32(singles + 0xc8), c.f(31)));
                    const auto word = c.u16(c.u32(cursor)); const auto value = std::uint32_t(word) + c.u32(seed);
                    c.put64(frame.at(0x84), fp.fctiwz(1, c.f(1))); c.put64(frame.at(0x74), fp.fctiwz(0, c.f(0)));
                    c.put32(frame.at(0x90), c.u32(frame.at(0x88)) ^ 0x80000000); c.put32(frame.at(0x94), 0x43300000);
                    c.set_f(3, c.f64(doubles + 0x80)); c.set_f(2, c.f64(doubles));
                    c.set_f(0, fp.fsub(c.f64(frame.at(0x8c)), c.f(3))); c.set_f(1, c.f64(doubles + 0x80));
                    c.set_f(2, fp.fdiv(c.f(0), c.f(2)));
                    const auto quotient = std::int32_t(value) / 65535, residual = std::int32_t(value) % 65535;
                    const auto sign = shifted_sign(std::int32_t(std::uint32_t(high_product(value, std::int32_t(0x80008001))) + value), 15);
                    c.put32(frame.at(0x98), std::uint32_t(residual) ^ 0x80000000);
                    c.put64(frame.at(0x7c), fp.fctiwz(0, fp.fmul(c.f(2), fp.fsub(c.f64(frame.at(0x94)), c.f(1)))));
                    c.set(17, c.u32(frame.at(0x80)));
                    const auto multiplied = c.u32(frame.at(0x78)) * 10u, reduced = std::uint32_t(std::int32_t(multiplied) / 65535);
                    shifted_sign(std::int32_t(std::uint32_t(high_product(multiplied, std::int32_t(0x80008001))) + multiplied), 15);
                    leave(c, {{3, reduced}, {4, 0x80008001}, {5, std::uint32_t(quotient) * 65535u}, {6, 65535}, {7, sign}, {8, word}});
                    enter(c, 0x102842cc, {reduced}); sample_range(c, reduced);
                    const auto numerator = c.r(3) + 5; const auto correction = std::uint32_t(std::int32_t(numerator) / 10);
                    c.put64(frame.at(0x6c), fp.fctiwz(0, fp.fmuls(c.f32(singles + 0xc8), c.f(31))));
                    c.set(4, shifted_sign(high_product(numerator, 0x66666667), 2)); c.set(3, c.u32(frame.at(0x70))); c.set(17, c.r(17) + correction);
                    if (compare(c, std::int32_t(c.r(17)), std::int32_t(c.r(3) - 1)) > 0) {
                        c.put64(frame.at(0x64), fp.fctiwz(0, fp.fmuls(c.f32(singles + 0xc8), c.f(31))));
                        c.set(3, c.u32(frame.at(0x68))); c.set(17, c.r(3) - 1);
                    }
                }
            }
            if (compare(c, std::int32_t(std::uint32_t(std::int8_t(c.r(21))) + c.r(19)), std::int32_t(c.r(17))) > 0) { move_to_ball(0x102843b4); return; }
        }
    }
    if (compare_logical(c, c.u32(player + 0x1b0)) != 0) {
        bool process_link = true;
        c.set(3, actor.match() + 0x10000 + std::uint32_t(std::int32_t(std::int8_t(actor.side()))) * 0x18e0);
        if (compare(c, c.u32(c.r(3) - 0x7e0c) & 0x400) != 0) {
            c.set(3, actor.match() + 0x10000);
            if (compare_logical(c, c.u32(c.r(3) - 0x2000)) != 0) {
                bool test_second = true;
                if (compare(c, std::int8_t(actor.side())) == 0 && compare(c, actor.row(), 6) < 0) {
                    c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(28))) - 1));
                    if (compare(c, std::int32_t(c.r(3)), actor.row()) < 0) test_second = false;
                    else {
                        c.set(4, c.u32(player + 0x1b0)); c.set(3, actor.match());
                        c.set(3, std::uint32_t(std::int32_t(c.i8(c.r(3) + 0x7925))));
                        if (compare(c, std::int32_t(c.r(3)), c.i8(c.r(4) + 0x106)) <= 0) test_second = false;
                    }
                }
                if (test_second) {
                    process_link = compare(c, actor.side(), 1) == 0 && compare(c, actor.row(), 5) > 0;
                    if (process_link) {
                        c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(28))) + 1));
                        if (compare(c, std::int32_t(c.r(3)), actor.row()) <= 0) {
                            c.set(4, c.u32(player + 0x1b0)); c.set(3, actor.match());
                            c.set(3, std::uint32_t(std::int32_t(c.i8(c.r(3) + 0x7926))));
                            process_link = compare(c, std::int32_t(c.r(3)), c.i8(c.r(4) + 0x106)) >= 0;
                        }
                    }
                }
            }
        }
        if (process_link) {
            c.set(3, c.u32(player + 0x1b0)); const auto slot = c.i8(c.r(3) + 0x19);
            bool keep_link = compare(c, slot, -4) == 0;
            if (!keep_link) {
                if (compare(c, slot, -4) < 0) keep_link = compare(c, slot, -5) < 0;
                else keep_link = compare(c, slot) >= 0;
            }
            if (!keep_link) c.put32(player + 0x1b0, 0);
            else {
                if (compare(c, c.i8(player + 0x108)) != 0) {
                    c.set(3, actor.match() + 0x10000); const auto linked = c.u32(player + 0x1b0);
                    c.set(3, c.u32(c.r(3) - 0x2008));
                    if (compare_logical(c, c.r(3), linked) == 0) {
                        c.put16(player + 0x19c, 0x105); move(c.r(27), c.r(28), UINT32_MAX, 0x1028452c); return;
                    }
                }
                c.put16(player + 0x19c, 0x105);
                const auto linked = c.u32(player + 0x1b0);
                c.set(14, c.u8(linked + 0x106)); c.set(13, c.u8(linked + 0x105));
                leave(c, {{3, actor.match()}, {4, unsigned(std::int8_t(actor.side()) == 0)}, {5, c.r(14)}}); c.call(0x101ee9f0, 0x10284568);
                if (compare(c, std::int8_t(c.r(3))) == 0 && compare(c, std::int8_t(c.r(18))) == 0) {
                    advance(0x10284598); const auto value = sample(); const auto sign = shifted_sign(high_product(value, 0x30c30c31), 2);
                    leave(c, {{3, sign}, {4, 0x30c30c31}, {5, value}, {6, value - c.u32(seed)}});
                    c.set_f(2, c.f32(c.u32(player + 0x1b0) + 0xb0)); c.set_f(1, c.f64(doubles + 0x80));
                    c.put32(frame.at(0x64), 0x43300000); c.put32(frame.at(0x68), std::uint32_t(std::int32_t(value) % 21) ^ 0x80000000);
                    c.set_f(0, fp.fsubs(c.f64(frame.at(0x64)), c.f(1))); fp.fcmp(0, c.f(2), c.f(0));
                    if (c.cr(0) & 8) {
                        if (compare(c, std::int8_t(actor.side())) == 0 && compare(c, std::int8_t(c.r(14))) > 0) c.set(14, c.r(14) - 1);
                        else if (compare(c, actor.side(), 1) == 0 && compare(c, std::int8_t(c.r(14)), 11) < 0) c.set(14, c.r(14) + 1);
                        if (compare(c, std::int8_t(c.r(13)), 2) <= 0) { compare(c, std::int8_t(c.r(13)), 8); c.set(13, c.r(13) + 1); }
                        else if (compare(c, std::int8_t(c.r(13)), 6) >= 0) { compare(c, std::int8_t(c.r(13))); c.set(13, c.r(13) - 1); }
                    }
                }
                move(c.r(13), c.r(14), UINT32_MAX, 0x10284688); return;
            }
        }
    }
    std::uint32_t target_back = 0x10284d70, bias_back = 0x10284d7c;
    bool use_ball_target = false;
    c.set(3, actor.match());
    if (compare(c, c.i8(c.r(3) + 0x7924), -1) != 0 && compare(c, c.r(22) & 1) == 0) {
        c.set_f(1, c.f64(doubles + 0x28));
        c.put64(frame.at(0x64), fp.fctiwz(0, fp.fadd(c.f(1), c.f32(player + 0xe4)))); c.set(17, c.u32(frame.at(0x68)));
        const auto bound = std::int32_t(c.r(17));
        if (compare(c, bound) == 0) c.set(18, 0);
        else {
            advance(0x102846f0); c.set(3, 65535); bool narrow = compare(c, bound, 65535) <= 0;
            if (narrow) { c.set(3, 0xffff0001); narrow = compare(c, bound, -65535) >= 0; }
            if (narrow) {
                c.set(3, sample()); compare(c, bound); const auto residual = std::int32_t(std::int64_t(std::int32_t(c.r(3))) % bound);
                c.set(18, bound > 0 ? std::uint32_t(residual) : 0u - std::uint32_t(residual));
            } else {
                c.put32(frame.at(0x64), 0x43300000); c.put32(frame.at(0x68), c.r(17) ^ 0x80000000);
                c.set_f(2, c.f64(doubles + 0x80)); c.set_f(1, fp.fsub(c.f64(frame.at(0x64)), c.f(2))); c.set_f(2, fp.fdiv(c.f(1), c.f64(doubles)));
                const auto word = c.u16(c.u32(cursor)); const auto value = std::uint32_t(word) + c.u32(seed);
                const auto residual = std::int32_t(value) % 65535;
                const auto sign = shifted_sign(std::int32_t(std::uint32_t(high_product(value, std::int32_t(0x80008001))) + value), 15);
                c.put32(frame.at(0x6c), 0x43300000); c.put32(frame.at(0x70), std::uint32_t(residual) ^ 0x80000000);
                c.set_f(1, c.f64(doubles + 0x80));
                c.put64(frame.at(0x74), fp.fctiwz(0, fp.fmul(c.f(2), fp.fsub(c.f64(frame.at(0x6c)), c.f(1))))); c.set(18, c.u32(frame.at(0x78)));
                const auto multiplied = c.r(17) * 10u, reduced = std::uint32_t(std::int32_t(multiplied) / 65535);
                shifted_sign(std::int32_t(std::uint32_t(high_product(multiplied, std::int32_t(0x80008001))) + multiplied), 15);
                leave(c, {{3, reduced}, {4, std::uint32_t(residual) ^ 0x80000000}, {5, 65535}, {6, sign}, {7, value}, {8, word}});
                enter(c, 0x102847c8, {reduced}); sample_range(c, reduced);
                const auto numerator = c.r(3) + 5; c.set(4, shifted_sign(high_product(numerator, 0x66666667), 2));
                c.set(3, std::uint32_t(std::int32_t(numerator) / 10)); c.set(18, c.r(18) + c.r(3));
                if (compare(c, std::int32_t(c.r(18)), std::int32_t(c.r(17) - 1)) > 0) c.set(18, c.r(17) - 1);
            }
        }
        if (compare(c, std::int32_t(c.r(18))) == 0) c.set_f(30, c.f64(doubles + 0x88));
        else {
            advance_with(0x10284864, [&] { c.set(17, c.u8(player + 0x110)); c.set_f(30, c.f64(doubles + 0x90)); });
            const auto value = sample(); const auto sign = shifted_sign(high_product(value, 0x66666667), 2);
            leave(c, {{3, std::uint32_t(std::int32_t(value) % 10)}, {4, sign}, {5, value}});
            if (compare(c, std::int8_t(c.r(17)), std::int32_t(c.r(3))) > 0) { c.set_f(0, c.f64(doubles + 0x3a0)); c.set_f(30, fp.fmul(c.f(30), c.f(0))); }
            if (compare(c, c.i8(player + 0x109)) == 0) { c.set_f(0, c.f64(doubles + 0x3a0)); c.set_f(30, fp.fmul(c.f(30), c.f(0))); }
            if (compare(c, c.i8(player + 0x10d)) == 0) { c.set_f(0, c.f64(doubles + 0x3a0)); c.set_f(30, fp.fmul(c.f(30), c.f(0))); }
            advance_with(0x102848fc, [&] { c.set_f(31, c.f32(player + 0x1a8)); });
            const auto second = sample(); const auto second_sign = shifted_sign(high_product(second, 0x1b4e81b5), 4);
            leave(c, {{3, second_sign}, {4, second}, {5, second - c.u32(seed)}});
            c.put32(frame.at(0x64), 0x43300000); c.put32(frame.at(0x68), (std::uint32_t(std::int32_t(second) % 150) + 30) ^ 0x80000000);
            c.set_f(1, c.f64(doubles + 0x80)); c.set_f(0, fp.fsubs(c.f64(frame.at(0x64)), c.f(1))); c.set(3, std::uint32_t(std::int32_t(second) % 150));
            fp.fcmp(0, c.f(31), c.f(0));
            if (c.cr(0) & 4) { c.set_f(0, c.f64(doubles + 0x3a0)); c.set_f(30, fp.fmul(c.f(30), c.f(0))); }
        }
        c.set(3, std::uint32_t(std::int32_t(actor.column()) - std::int8_t(c.r(27)))); c.call(0x10775540, 0x10284978);
        c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(28))))); c.set(17, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
        c.set(3, std::uint32_t(std::int32_t(actor.row()) - std::int8_t(c.r(28)))); c.call(0x10775540, 0x10284990);
        c.set(3, std::uint32_t(std::int8_t(c.r(3)) * 36)); c.set_f(0, c.f32(distances + c.r(3) + (std::uint32_t(std::int8_t(c.r(17))) << 2)));
        fp.fcmp(0, c.f(0), c.f(30));
        target_back = 0x10284d3c; bias_back = 0x10284d48;
        if (c.cr(0) & 8) {
            const auto policy = [&](std::uint32_t back, std::uint32_t mask, bool first) {
                const auto match = actor.match(); const auto side = std::uint32_t(std::int32_t(std::int8_t(actor.side())));
                leave(c, {{3, match + side * 0x18e0 + 0x7c6c}, {4, c.u8(player + 0x19)}, {5, match}}); c.call(0x10113e10, back);
                const auto bits = c.r(4) & mask; leave(c, {{3, bits}, {4, 0}, {5, 0}, {6, bits}}); if (first) c.set(7, 0);
                return compare(c, bits) != 0;
            };
            const auto team_has = [&](std::uint32_t mask) {
                c.set(4, actor.match()); c.set(3, c.r(4) + std::uint32_t(std::int32_t(std::int8_t(actor.side()))) * 0x18e0 + 0x7c6c);
                return compare(c, c.u32(c.r(3) + 0x588) & mask) != 0;
            };
            use_ball_target = policy(0x102849d0, 0x8000, true);
            if (!use_ball_target) {
                const bool policy2000 = policy(0x10284a1c, 0x2000, false);
                if (policy2000) use_ball_target = team_has(0x100);
                if (!use_ball_target) use_ball_target = team_has(0x4000);
            }
            target_back = 0x10284d08; bias_back = 0x10284d14;
            if (!use_ball_target && !role_has(0x10284aa4, 2) && !role_has(0x10284ab4, 4)) {
                c.set(3, actor.match() + unsigned(std::int8_t(actor.side()) == 0)); c.set(18, c.u8(c.r(3) + 0x77bc));
                if (compare(c, std::int8_t(c.r(18))) == 0) c.set(17, 0);
                else {
                    advance(0x10284b0c); c.set(3, 65535); compare(c, std::int8_t(c.r(18)), 65535);
                    c.set(3, 0xffff0001); compare(c, std::int8_t(c.r(18)), -65535);
                    const auto address = c.u32(cursor), value = sample();
                    leave(c, {{3, std::uint32_t(std::int32_t(std::int8_t(c.r(18))))}, {4, value}, {5, address}}); compare(c, std::int8_t(c.r(18)));
                    const auto residual = std::int32_t(std::int64_t(std::int32_t(value)) % std::int8_t(c.r(18)));
                    c.set(17, std::int8_t(c.r(18)) > 0 ? std::uint32_t(residual) : 0u - std::uint32_t(residual));
                }
                advance(0x10284c84); const auto value = sample(); const auto sign = shifted_sign(high_product(value, 0x51eb851f), 3);
                leave(c, {{3, sign}, {4, value}});
                if (compare(c, std::int32_t(c.r(17)), std::int32_t(value) % 25) > 0) {
                    leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101f0dd0, 0x10284cd0); use_ball_target = compare(c, std::int8_t(c.r(3))) == 0;
                }
            }
        }
    }
    if (use_ball_target) { c.put8(frame.at(0x3b), c.r(27)); c.set(20, 1); c.put8(frame.at(0x3a), c.r(28)); }
    else {
        // R16 is initialized to0 and its first later assignment is the keeper
        // lookup after this phase. Retain the source comparison's CR effects.
        compare(c, std::int8_t(c.r(16))); prepare_target(target_back, bias_back);
    }
    if (compare(c, std::int16_t(c.r(22)), 1) != 0) {
        c.set(3, actor.match());
        if (compare(c, c.i8(c.r(3) + 0x7924), -1) != 0 && own_goal_area()) {
            leave(c, {{3, actor.match()}, {4, actor.side()}, {5, c.r(27)}, {6, c.r(28)}}); c.call(0x101eea30, 0x10284e60);
            if (compare(c, std::int8_t(c.r(3))) != 0) {
                leave(c, {{3, actor.match()}, {4, actor.side()}}); c.call(0x101f1e10, 0x10284e74); c.set(16, c.r(3));
                if (compare(c, std::int32_t(c.r(16))) == 0) {
                    leave(c, {{3, strings + 0x8f}, {4, 0}, {5, 0}, {6, frame.at(0x44)}, {7, frame.at(0x3c)}}); c.call(0x10739890, 0x10284e94);
                    c.call(0x10768810, 0x10284e98);
                    leave(c, {{3, messages}, {4, strings + 0x9c}, {5, c.r(3)}, {6, frame.at(0x44)}, {7, frame.at(0x3c)}, {8, 0x540c}}); c.call(0x1077b720, 0x10284eb4);
                    leave(c, {{3, strings + 0xa9}, {4, messages}, {5, 0}}); c.call(0x10769b60, 0x10284ec4);
                    c.set(4, c.toc(-0x7bac)); c.set(3, UINT32_MAX); c.put8(c.r(4), 0); c.call(0x1076f7d0, 0x10284ed8);
                }
                if (compare(c, c.i8(c.r(16) + 0x2c), 1) > 0) {
                    move(4, actor.side() == 0 ? 0 : 11, UINT32_MAX, 0x10284f20); c.put16(player + 0x19c, 0x78); return;
                }
            }
        }
    }
    if (compare(c, std::int16_t(c.r(22)), 1) == 0) {
        c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(27)))));
        bool on_ball = compare(c, std::int32_t(c.r(3)), actor.column()) == 0;
        if (on_ball) { c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(28))))); on_ball = compare(c, std::int32_t(c.r(3)), actor.row()) == 0; }
        if (on_ball) {
            c.set(3, actor.match()); if (compare(c, c.i8(c.r(3) + 0x7924), -1) != 0) return;
            c.set(3, actor.match()); if (compare(c, c.i8(c.r(3) + 0x791f), 5) >= 0) return;
            c.set(3, actor.match()); if (compare(c, c.i8(c.r(3) + 0x1de)) != 0) return;
            reset_heading(0x10284fcc); publish(0x1f46, 0x10284ff4);
            enter(c, 0x10284ffc, {player}); acquire_ball(c, player);
            c.set(3, extended(c.i16(player + 0x36)) + 4); c.put16(player + 0x36, c.r(3)); c.put16(player + 0x19c, 0x103); return;
        }
        leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101f5280, 0x10285020);
        if (compare(c, std::int8_t(c.r(3))) != 0) {
            c.set(3, std::uint32_t(std::int32_t(actor.column()) - std::int8_t(c.r(27)))); c.call(0x10775540, 0x1028503c);
            c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(28))))); c.set(13, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
            c.set(3, std::uint32_t(std::int32_t(actor.row()) - std::int8_t(c.r(28)))); c.call(0x10775540, 0x10285054);
            c.set(3, std::uint32_t(std::int8_t(c.r(3)) * 36)); c.set_f(0, c.f64(doubles + 0x90));
            c.set_f(1, c.f32(distances + c.r(3) + (std::uint32_t(std::int8_t(c.r(13))) << 2)));
            fp.fcmp(0, c.f(1), c.f(0)); const auto bits = c.cr(0); c.set_cr(0, (bits & ~2u) | ((bits & 10) ? 2 : 0));
            if (c.cr(0) & 2) {
                leave(c, {{3, actor.match()}, {4, actor.side()}, {5, c.r(27)}, {6, c.r(28)}}); c.call(0x101eea30, 0x10285090);
                if (compare(c, std::int8_t(c.r(3))) != 0) {
                    c.set(3, actor.match()); bool take_ball = compare(c, c.i8(c.r(3) + 0x7924), -1) == 0;
                    if (!take_ball) take_ball = compare(c, c.i8(player + 0x109)) == 0 && compare(c, c.i8(player + 0x10d)) == 0;
                    if (take_ball) { c.put16(player + 0x19c, 0x103); move(c.r(27), c.r(28), 0x100, 0x102850e8); return; }
                }
                c.set(3, actor.match());
                if (compare(c, c.i8(c.r(3) + 0x7924), -1) == 0) {
                    leave(c, {{3, actor.match()}, {4, unsigned(std::int8_t(actor.side()) == 0)}, {5, c.r(27)}, {6, c.r(28)}, {7, 1}}); c.call(0x101e3f10, 0x10285128);
                    c.set_f(0, c.f64(doubles + 0xc0)); fp.fcmp(0, c.f(1), c.f(0));
                    bool take_ball = (c.cr(0) & 4) != 0;
                    if (!take_ball) take_ball = compare(c, c.i8(player + 0x109)) == 0 && compare(c, c.i8(player + 0x10d)) == 0;
                    if (take_ball) {
                        if (compare(c, std::int8_t(c.r(27))) < 0 || compare(c, std::int8_t(c.r(27)), 9) >= 0 ||
                            compare(c, std::int8_t(c.r(28))) < 0 || compare(c, std::int8_t(c.r(28)), 12) >= 0) return;
                        c.put16(player + 0x19c, 0x103); move(c.r(27), c.r(28), 0x100, 0x10285198); return;
                    }
                }
                c.put16(player + 0x19c, 0x71); move(c.u8(frame.at(0x3b)), c.u8(frame.at(0x3a)), UINT32_MAX, 0x102851c0); return;
            }
        }
        c.put16(player + 0x19c, 0x71); move(c.u8(frame.at(0x3b)), c.u8(frame.at(0x3a)), UINT32_MAX, 0x102851e8); return;
    }
    leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101e4e10, 0x102851f8);
    if (compare(c, std::int8_t(c.r(3))) >= 0 && compare(c, c.i8(player + 0x10a)) != 0) {
        enter(c, 0x10285218, {player, 0}); advance_row(c, player, 0); c.put16(player + 0x19c, 0x6b); return;
    }
    leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101f5280, 0x10285230);
    if (compare(c, std::int8_t(c.r(3))) != 0) {
        c.set(3, actor.match()); bool chase_ball = compare(c, c.i8(c.r(3) + 0x7924), -1) == 0 && pursuit_half();
        if (!chase_ball) {
            c.set(3, actor.match()); const bool loose = compare(c, c.i8(c.r(3) + 0x7924), -1) == 0;
            // With no intervening callback or write, this repeated half test
            // has the same false result as above. Keep its CR effects.
            if (loose) pursuit_half();
            else chase_ball = compare(c, c.i8(player + 0x110)) != 0;
            if (!chase_ball && compare(c, c.i8(player + 0x110)) != 0) {
                leave(c, {{3, actor.match()}, {4, c.r(27)}, {5, c.r(28)}, {6, c.u8(frame.at(0x3b))}, {7, c.u8(frame.at(0x3a))}}); c.call(0x101ef8d0, 0x10285328);
                c.set_f(0, c.f32(singles + 0x2c)); fp.fcmp(0, c.f(1), c.f(0)); chase_ball = (c.cr(0) & 8) != 0;
            }
        }
        if (chase_ball) {
            const bool valid = compare(c, std::int8_t(c.r(27))) >= 0 && compare(c, std::int8_t(c.r(27)), 9) < 0 &&
                               compare(c, std::int8_t(c.r(28))) >= 0 && compare(c, std::int8_t(c.r(28)), 12) < 0;
            if (valid) {
                c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(27)))));
                bool on_ball = compare(c, std::int32_t(c.r(3)), actor.column()) == 0;
                if (on_ball) { c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(28))))); on_ball = compare(c, std::int32_t(c.r(3)), actor.row()) == 0; }
                if (on_ball) {
                    c.set(3, actor.match()); bool acquire = compare(c, c.i8(c.r(3) + 0x7924), -1) == 0;
                    if (acquire) { c.set(3, actor.match()); acquire = compare(c, c.i8(c.r(3) + 0x791f), 5) < 0; }
                    if (acquire) { c.set(3, actor.match()); acquire = compare(c, c.i8(c.r(3) + 0x1de)) == 0; }
                    if (acquire) {
                        c.put16(player + 0x19c, 0x100); c.set(4, extended(c.i16(player + 0x36)) + 4); c.put16(player + 0x36, c.r(4));
                        reset_heading(0x10285404);
                        c.set(3, actor.match()); leave(c, {{4, actor.side()}, {5, c.u8(player + 0x105)}, {6, c.u8(player + 0x106)}});
                        bool special = compare(c, c.i8(c.r(3) + 0x1e6)) != 0 && compare(c, std::int8_t(c.r(4))) == 0 &&
                                       compare(c, std::int8_t(c.r(6)), 10) >= 0 && compare(c, std::int8_t(c.r(5)), 5) <= 0 && compare(c, std::int8_t(c.r(5)), 3) >= 0;
                        if (!special) special = compare(c, c.i8(c.r(3) + 0x1e6)) != 0 && compare(c, std::int8_t(c.r(4)), 1) == 0 &&
                                                compare(c, std::int8_t(c.r(6)), 1) <= 0 && compare(c, std::int8_t(c.r(5)), 5) <= 0 && compare(c, std::int8_t(c.r(5)), 3) >= 0;
                        compare(c, special); publish(special ? 0x2113 : 0x1f46, special ? 0x102854cc : 0x102854f8);
                        enter(c, 0x10285500, {player}); acquire_ball(c, player); return;
                    }
                    c.put16(player + 0x19c, 0x104); return;
                }
                c.put16(player + 0x19c, 0x100); move(c.r(27), c.r(28), 0x100, 0x10285534); return;
            }
            const auto use_ball = compare(c, std::int8_t(c.r(20))) != 0;
            c.put16(player + 0x19c, use_ball ? 0x100 : 0x68); move(c.u8(frame.at(0x3b)), c.u8(frame.at(0x3a)), 0x100, 0x10285568); return;
        }
    }
    if (own_goal_area()) {
        const auto cell_count = [&](std::uint32_t side) {
            c.set(5, actor.match()); c.set(4, c.r(5) + std::uint32_t(std::int32_t(actor.column())) * 0x450);
            c.set(3, c.r(4) + std::uint32_t(std::int32_t(actor.row())) * 0x5c + side); return c.u8(c.r(3) + 0xa60);
        };
        if (compare(c, std::int8_t(cell_count(unsigned(std::int8_t(actor.side()) == 0)))) != 0 &&
            compare(c, cell_count(std::uint32_t(std::int32_t(std::int8_t(actor.side())))), 1) == 0 && compare(c, c.i8(player + 0x10d)) != 0) {
            c.put16(player + 0x19c, 0x104); return;
        }
    }
    c.set(3, std::uint32_t(std::int32_t(c.i8(frame.at(0x3b)))));
    bool same_target = compare(c, std::int32_t(c.r(3)), actor.column()) == 0;
    if (same_target) { c.set(3, std::uint32_t(std::int32_t(c.i8(frame.at(0x3a))))); same_target = compare(c, std::int32_t(c.r(3)), actor.row()) == 0; }
    const auto ball_target = compare(c, std::int8_t(c.r(20))) != 0;
    if (!same_target) {
        c.put16(player + 0x19c, ball_target ? 0x100 : 0x68); move(c.u8(frame.at(0x3b)), c.u8(frame.at(0x3a)), 0x100, 0x10285714); return;
    }
    c.put16(player + 0x19c, ball_target ? 0x100 : 0x76);
}
}
