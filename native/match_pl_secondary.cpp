#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void choose_secondary_action(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x10281af0, 0xc0, {.first_gpr = 19});
    const auto messages = c.toc(-0x7ba8), doubles = c.toc(-0x51e8), seed = c.toc(-0x7b90),
               begin = c.toc(-0x7b8c), end = c.toc(-0x7b88), cursor = c.toc(-0x7b84), strings = c.toc(-0x51e4);
    keep(c, {{22, messages}, {23, strings}, {24, 0}, {25, end}, {26, begin}, {28, doubles}, {29, seed}, {30, cursor}, {31, player}});
    const Player actor{c, player};
    Fpu fp(c);
    const auto high_product = [](std::uint32_t value, std::int32_t multiplier) {
        return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * multiplier) >> 32);
    };
    const auto shifted_sign = [&](std::int32_t high, unsigned bits) {
        c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << bits) - 1)));
        return std::uint32_t(high < 0);
    };
    const auto advance = [&](std::uint32_t back) {
        const auto next = c.u32(cursor) + 2; c.set(3, next);
        const auto order = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
        if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
    };
    const auto sample = [&] { return std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed); };
    const auto role_has = [&](std::uint32_t back, std::uint32_t mask) {
        enter(c, back, {player}); current_role(c, player);
        return compare(c, c.r(3) & mask) != 0;
    };
    const auto move = [&](std::uint32_t column, std::uint32_t row, std::uint32_t kind, std::uint32_t back) {
        leave(c, {{3, player}, {4, column}, {5, row}, {6, kind}, {7, 0}, {8, 0}});
        c.call(0x1022e7e0, back);
    };
    const auto offer_move = [&](std::uint32_t mode, std::uint32_t query_back, std::uint32_t move_back,
                                unsigned action, bool improve) {
        leave(c, {{3, actor.match()}, {4, player}, {5, frame.at(0x39)}, {6, frame.at(0x38)}, {7, mode}});
        c.call(0x101d8260, query_back);
        if (compare(c, std::int8_t(c.r(3))) == 0) return false;
        c.put16(player + 0x19c, action);
        if (improve) c.put16(player + 0x36, std::uint32_t(c.i16(player + 0x36)) + 7);
        move(c.u8(frame.at(0x39)), c.u8(frame.at(0x38)), UINT32_MAX, move_back);
        return true;
    };
    const auto same_position_move = [&](std::uint32_t back) {
        c.put16(player + 0x19c, 0x76);
        move(c.u8(player + 0x105), c.u8(player + 0x106), UINT32_MAX, back);
    };
    const auto condition_outweighs_draw = [&](unsigned trait_reg, unsigned draw_reg) {
        const auto condition = extended(c.i16(player + 0x36));
        const auto count = c.u8(player + 0x104);
        const auto signed_count = std::int32_t(std::int8_t(count));
        const auto trait = std::int32_t(std::int8_t(c.r(trait_reg))), product = signed_count * trait;
        leave(c, {{3, c.r(draw_reg) ^ 0x80000000}, {4, std::uint32_t(product) ^ 0x80000000},
                  {5, std::uint32_t(product)}, {6, std::uint32_t(signed_count)}, {7, 0x43300000}});
        c.put32(frame.at(0x64), 0x43300000); c.put32(frame.at(0x68), condition ^ 0x80000000);
        c.set_f(2, c.f64(doubles + 0x80));
        c.set_f(1, fp.fsub(c.f64(frame.at(0x64)), c.f(2)));
        c.set_f(3, c.f64(doubles + 0xc0));
        c.set_f(2, fp.fdiv(c.f(1), c.f64(doubles + 0x398)));
        c.put32(frame.at(0x6c), 0x43300000); c.put32(frame.at(0x70), std::uint32_t(product) ^ 0x80000000);
        c.set_f(1, c.f64(doubles + 0x80));
        const auto product_value = c.f64(frame.at(0x6c));
        c.put32(frame.at(0x74), 0x43300000); c.put32(frame.at(0x78), c.r(draw_reg) ^ 0x80000000);
        c.set_f(3, fp.fsub(c.f(3), c.f(2))); c.set_f(2, fp.fsubs(product_value, c.f(1)));
        c.set_f(1, c.f64(doubles + 0x80)); c.set_f(0, c.f64(frame.at(0x74)));
        c.set_f(3, fp.frsp(c.f(3))); c.set_f(0, fp.fsubs(c.f(0), c.f(1)));
        c.set_f(1, fp.fmuls(c.f(3), c.f(2))); fp.fcmp(0, c.f(1), c.f(0));
        return (c.cr(0) & 4) != 0;
    };
    const auto opposite_goal_area = [&] {
        const auto other_side = unsigned(std::int8_t(actor.side()) == 0);
        const auto match = actor.match();
        c.set(3, match); c.set(4, c.u8(match + 0x791e)); c.set(5, c.u8(match + 0x791d));
        c.set(3, 0); bool inside = false;
        if (compare(c, other_side, 1) == 0) {
            if (compare(c, std::int8_t(c.r(5)), 2) >= 0 && compare(c, std::int8_t(c.r(5)), 6) <= 0 &&
                compare(c, std::int8_t(c.r(4)), 10) >= 0 && compare(c, std::int8_t(c.r(4)), 11) <= 0) inside = true;
        } else if (compare(c, std::int8_t(c.r(5)), 2) >= 0 && compare(c, std::int8_t(c.r(5)), 6) <= 0 &&
                   compare(c, std::int8_t(c.r(4))) >= 0 && compare(c, std::int8_t(c.r(4)), 1) <= 0) inside = true;
        c.set(3, inside); compare(c, inside); return inside;
    };

    leave(c, {{3, actor.match() + std::uint32_t(std::int8_t(actor.side()) * 0x18e0) + 0x7c6c}, {4, c.u8(player + 0x19)}});
    c.call(0x101149f0, 0x10281b44);
    if (compare(c, c.r(3) & 4) != 0) c.set(24, 1);
    c.put16(player + 0x19c, UINT32_MAX); c.set(3, actor.match());
    if (compare(c, c.i8(c.r(3) + 0x791f), -1) == 0) {
        c.set(3, actor.match());
        if (compare(c, c.i8(c.r(3) + 0x7924), -1) != 0) {
            leave(c, {{3, actor.match()}, {4, player}, {5, frame.at(0x39)}, {6, frame.at(0x38)}});
            c.call(0x101e25a0, 0x10281b94);
            c.put16(player + 0x19c, 0x68);
            enter(c, 0x10281bac, {player, c.u8(frame.at(0x39)), c.u8(frame.at(0x38))});
            set_coordinates(c, player, c.r(4), c.r(5)); return;
        }
    }
    c.set(3, actor.match());
    if (compare(c, c.i8(c.r(3) + 0x7924), -1) == 0) {
        c.set(3, actor.match() + 0x10000);
        if (compare_logical(c, c.u32(c.r(3) - 0x2008), player) == 0) {
            c.set(27, c.u8(player + 0x10a)); advance(0x10281bfc); const auto value = sample();
            const auto sign = shifted_sign(high_product(value, 0x66666667), 3), residual = std::uint32_t(std::int32_t(value) % 20);
            leave(c, {{3, residual}, {4, sign}, {5, value}});
            if (compare(c, std::int8_t(c.r(27)) + std::int32_t(residual)) != 0) {
                const auto match = actor.match(); move(c.u8(match + 0x791d), c.u8(match + 0x791e), 0x100, 0x10281c64);
                c.put16(player + 0x19c, 0x100); return;
            }
        }
        c.set(3, actor.match() + 0x10000);
        if (compare_logical(c, c.u32(c.r(3) - 0x2008)) == 0) {
            leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101f5280, 0x10281c90);
            if (compare(c, std::int8_t(c.r(3))) != 0) {
                c.set(3, actor.match());
                if (compare(c, c.i8(c.r(3) + 0x1de)) == 0) {
                    const auto match = actor.match(); move(c.u8(match + 0x791d), c.u8(match + 0x791e), 0x100, 0x10281ccc);
                    c.put16(player + 0x19c, 0x100); return;
                }
            }
        }
    }
    bool near = compare(c, actor.side(), 1) == 0 && compare(c, actor.row(), 3) < 0;
    if (!near) near = compare(c, std::int8_t(actor.side())) == 0 && compare(c, actor.row(), 8) > 0;
    compare(c, near);
    if (near) {
        const bool wide = compare(c, actor.column(), 2) < 0 || compare(c, actor.column(), 6) > 0;
        compare(c, wide);
        if (wide) {
            c.set(3, actor.match()); c.set(4, actor.match());
            c.set(3, std::uint32_t(std::int32_t(actor.column()) - c.i8(c.r(3) + 0x791d)));
            const auto saved_row = c.u8(actor.match() + 0x791e);
            c.set(19, saved_row); c.call(0x10775540, 0x10281d7c);
            c.set(19, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
            c.set(4, std::uint32_t(std::int32_t(std::int8_t(saved_row))));
            c.set(3, std::uint32_t(std::int32_t(actor.row()) - std::int8_t(saved_row)));
            c.call(0x10775540, 0x10281d94); c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
            c.set(4, c.toc(-0x6424)); c.set_f(0, c.f64(doubles + 0xc8));
            c.set_f(1, c.f32(c.r(4) + std::uint32_t(std::int8_t(c.r(3)) * 36) + (std::uint32_t(std::int8_t(c.r(19))) << 2)));
            fp.fcmp(0, c.f(1), c.f(0)); const auto bits = c.cr(0);
            c.set_cr(0, (bits & ~2u) | ((bits & 10) ? 2 : 0)); near = (c.cr(0) & 2) == 0;
        }
    }
    if (!near) {
        enter(c, 0x10281dc8, {player}); strategy_bit21(c, player);
        if (compare(c, std::int8_t(c.r(3))) != 0) {
            c.set(3, c.u32(player + 0x70)); c.set(21, std::uint32_t(std::int32_t(c.i8(c.r(3) + 0x1a))));
            if (compare(c, std::int32_t(c.r(21))) != 0) {
                advance(0x10281e08); c.set(3, 0xffff); compare(c, std::int32_t(c.r(21)), 65535);
                c.set(3, 0xffff0001); compare(c, std::int32_t(c.r(21)), -65535);
                c.set(3, sample()); compare(c, std::int32_t(c.r(21)));
                const auto residual = std::int32_t(std::int64_t(std::int32_t(c.r(3))) % std::int32_t(c.r(21)));
                c.set(27, std::uint32_t(residual));
                if (std::int32_t(c.r(21)) < 0) c.set(27, 0u - c.r(27));
            } else c.set(27, 0);
            advance(0x10281f64); const auto value = sample();
            const auto sign = shifted_sign(high_product(value, 0x66666667), 1);
            leave(c, {{3, sign}, {4, value}});
            if (compare(c, std::int32_t(c.r(27)), std::int32_t(value) % 5) >= 0 && offer_move(12, 0x10281fbc, 0x10281fe8, 0x6a, false)) return;
        }
        c.set(21, c.u8(player + 0x10a));
        const bool role = role_has(0x10281ff8, 0x40); c.set(27, role ? 20 : 50);
        // Both role-selected bounds are nonzero signed16 values.
        compare(c, std::int32_t(c.r(27))); advance(0x10282038);
        c.set(3, 65535); compare(c, std::int32_t(c.r(27)), 65535);
        c.set(3, 0xffff0001); compare(c, std::int32_t(c.r(27)), -65535);
        c.set(3, sample()); compare(c, std::int32_t(c.r(27))); c.set(20, std::uint32_t(std::int32_t(c.r(3)) % std::int32_t(c.r(27))));
        if (condition_outweighs_draw(21, 20) && offer_move(4, 0x10282214, 0x1028224c, 0x6a, true)) return;
    } else {
        c.set(20, c.u8(player + 0x10a));
        const bool role = role_has(0x1028225c, 0x40); c.set(21, role ? 20 : 50);
        compare(c, std::int32_t(c.r(21))); advance(0x1028229c);
        c.set(3, 65535); compare(c, std::int32_t(c.r(21)), 65535);
        c.set(3, 0xffff0001); compare(c, std::int32_t(c.r(21)), -65535);
        c.set(3, sample()); compare(c, std::int32_t(c.r(21))); c.set(27, std::uint32_t(std::int32_t(c.r(3)) % std::int32_t(c.r(21))));
        if (condition_outweighs_draw(20, 27) && offer_move(4, 0x10282478, 0x102824b0, 0x6a, true)) return;
    }
    leave(c, {{3, actor.match()}, {4, player}, {5, frame.at(0x3b)}, {6, frame.at(0x3a)}, {7, 1}});
    c.call(0x101e28b0, 0x102824cc);
    enter(c, 0x102824d8, {player, c.u8(frame.at(0x3b))}); adjust_action_bias(c, player, c.r(4));
    c.put8(frame.at(0x3b), c.r(3));
    enter(c, 0x102824e8, {player}); alternate_strategy_flags(c, player);
    leave(c, {{3, 0}, {4, 0}, {5, c.r(3) & 4}, {6, 0}});
    if (compare(c, c.r(5)) != 0) {
        const bool first_side = compare(c, std::int8_t(actor.side())) == 0;
        const auto role1 = first_side ? 0x10282524u : 0x1028270cu;
        const auto role2 = first_side ? 0x10282534u : 0x1028271cu;
        const auto role3 = first_side ? 0x10282618u : 0x10282800u;
        const auto role4 = first_side ? 0x10282628u : 0x10282810u;
        if (role_has(role1, 0x80) && role_has(role2, 0x78)) {
            if (!opposite_goal_area()) {
                c.set(3, actor.match());
                if (compare(c, c.i8(c.r(3) + 0x791d), first_side ? 6 : 2) < 0) {
                    c.set(3, first_side ? 0 : 8); c.put8(frame.at(0x3b), c.r(3)); c.put8(frame.at(0x3a), first_side ? 11 : 0);
                }
            }
        } else if (role_has(role3, 0x800) && role_has(role4, 0x78) && !opposite_goal_area()) {
            c.set(3, actor.match());
            if (compare(c, c.i8(c.r(3) + 0x791d), first_side ? 2 : 6) < 0) {
                c.set(3, first_side ? 8 : 0); c.put8(frame.at(0x3b), c.r(3)); c.put8(frame.at(0x3a), first_side ? 11 : 0);
            }
        }
    }
    c.set(27, 0); leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101e3d50, 0x102828f8);
    if (compare(c, c.r(3) & 0x200) != 0) {
        leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101e3d50, 0x1028290c);
        if (compare(c, c.r(3) & 8) != 0) c.set(27, 18);
        else {
            leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101e3d50, 0x10282928);
            if (compare(c, c.r(3) & 4) != 0) c.set(27, 20);
        }
    } else c.set(27, 16);
    enter(c, 0x10282944, {player}); team_or_slot_status(c, player);
    bool central_target = false;
    if (compare(c, std::int8_t(c.r(3))) != 0) {
        const bool has_bound = compare(c, std::int8_t(c.r(27))) != 0;
        c.set(20, c.u8(player + 0x10a));
        if (has_bound) {
            advance(0x10282980); c.set(3, 65535); compare(c, std::int8_t(c.r(27)), 65535);
            c.set(3, 0xffff0001); compare(c, std::int8_t(c.r(27)), -65535);
            const auto address = c.u32(cursor), value = sample();
            leave(c, {{3, std::uint32_t(std::int32_t(std::int8_t(c.r(27))))}, {4, value}, {5, address}});
            compare(c, std::int8_t(c.r(27))); c.set(21, std::uint32_t(std::int32_t(value) % std::int8_t(c.r(27))));
        } else c.set(21, 0);
        central_target = compare(c, std::int8_t(c.r(20)), std::int32_t(c.r(21))) > 0;
    }
    if (!central_target) {
        bool beyond = compare(c, std::int8_t(actor.side())) == 0;
        if (beyond) {
            c.set(3, std::uint32_t(std::int32_t(c.i8(actor.match() + 0x791e))));
            beyond = compare(c, std::int32_t(c.r(3)), actor.row()) > 0 && compare(c, actor.row(), 7) > 0;
        }
        if (!beyond && compare(c, actor.side(), 1) == 0) {
            c.set(3, std::uint32_t(std::int32_t(c.i8(actor.match() + 0x791e))));
            beyond = compare(c, std::int32_t(c.r(3)), actor.row()) < 0 && compare(c, actor.row(), 4) < 0;
        }
        if (beyond && compare(c, c.i8(player + 0x10c)) != 0) {
            advance(0x10282b80); const auto value = sample(); c.set(19, std::uint32_t(std::int32_t(value) % 3));
            const auto high = high_product(value, 0x55555556);
            leave(c, {{3, actor.match()}, {4, c.u8(player + 0x106)}, {5, actor.side()}, {6, 1}, {7, std::uint32_t(high)}, {8, value}});
            c.call(0x101ee680, 0x10282bc8); c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
            central_target = compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(19) + 1)) < 0;
        }
    }
    if (central_target) {
        const auto column = std::int8_t(c.u8(frame.at(0x3b))) + 4;
        const auto sign = std::uint32_t(column < 0);
        c.s.wr<1>(reg::xer_ca, column + std::int32_t(sign) < 0 && ((column + std::int32_t(sign)) & 1));
        c.set(3, actor.side() == 0 ? 11 : 0); c.set(4, 11); c.set(5, actor.side() == 0);
        c.put8(frame.at(0x3b), (std::uint32_t(column) + sign) >> 1); c.put8(frame.at(0x3a), c.r(3));
    }
    if (compare(c, c.i8(player + 0x10a)) != 0) {
        leave(c, {{3, actor.match()}, {4, actor.side()}, {5, c.u8(frame.at(0x3a))}}); c.call(0x101e4ea0, 0x10282c30);
        c.set(19, c.r(3));
        if (compare(c, std::int8_t(c.r(19))) >= 0) {
            const bool first_side = compare(c, std::int8_t(actor.side())) == 0;
            const auto row = c.u8(frame.at(0x3a)); c.put8(frame.at(0x3a), first_side ? row - c.r(19) : row + c.r(19));
        }
    }
    leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101e3d50, 0x10282c70); c.set(27, c.r(3));
    if (compare_logical(c, c.u32(player + 0x1b0)) != 0) {
        const auto linked = c.u32(player + 0x1b0); const auto slot = c.i8(linked + 0x19);
        bool invalid = compare(c, slot, -1) == 0;
        if (!invalid && compare(c, slot, -1) < 0) invalid = compare(c, slot, -5) == 0;
        if (invalid) {
            leave(c, {{3, strings + 0x8f}, {4, 0}, {5, 0}, {6, frame.at(0x44)}, {7, frame.at(0x3c)}}); c.call(0x10739890, 0x10282cbc);
            c.call(0x10768810, 0x10282cc0);
            leave(c, {{3, messages}, {4, strings + 0x9c}, {5, c.r(3)}, {6, frame.at(0x44)}, {7, frame.at(0x3c)}, {8, 0x521a}}); c.call(0x1077b720, 0x10282cdc);
            leave(c, {{3, strings + 0xa9}, {4, messages}, {5, 0}}); c.call(0x10769b60, 0x10282cec);
            c.set(3, c.toc(-0x7bac)); c.put8(c.r(3), 0);
        } else {
            c.put16(player + 0x19c, 0x105); c.set(20, c.u8(linked + 0x105)); c.set(21, c.u8(linked + 0x106));
            if (compare(c, c.i8(player + 0x10d)) != 0) {
                leave(c, {{3, actor.match()}, {4, unsigned(std::int8_t(actor.side()) == 0)}, {5, c.r(21)}}); c.call(0x101ee9f0, 0x10282d40);
                if (compare(c, std::int8_t(c.r(3))) == 0 && compare(c, std::int8_t(c.r(24))) == 0) {
                    advance(0x10282d70); const auto value = sample(), word = value - c.u32(seed);
                    const auto sign = shifted_sign(high_product(value, 0x30c30c31), 2);
                    leave(c, {{3, sign}, {4, 0x30c30c31}, {5, value}, {6, word}});
                    c.set_f(2, c.f32(c.u32(player + 0x1b0) + 0xb0)); c.set_f(1, c.f64(doubles + 0x80));
                    c.put32(frame.at(0x64), 0x43300000); c.put32(frame.at(0x68), std::uint32_t(std::int32_t(value) % 21) ^ 0x80000000);
                    c.set_f(0, fp.fsubs(c.f64(frame.at(0x64)), c.f(1))); fp.fcmp(0, c.f(2), c.f(0));
                    if (c.cr(0) & 8) {
                        if (compare(c, std::int8_t(actor.side())) == 0 && compare(c, std::int8_t(c.r(21))) > 0) c.set(21, c.r(21) - 1);
                        else if (compare(c, actor.side(), 1) == 0 && compare(c, std::int8_t(c.r(21)), 11) < 0) c.set(21, c.r(21) + 1);
                        // The source's second bounds tests are implied by the first;
                        // retain their CR effects without unreachable branches.
                        if (compare(c, std::int8_t(c.r(20)), 2) <= 0) { compare(c, std::int8_t(c.r(20)), 8); c.set(20, c.r(20) + 1); }
                        else if (compare(c, std::int8_t(c.r(20)), 6) >= 0) { compare(c, std::int8_t(c.r(20))); c.set(20, c.r(20) - 1); }
                    }
                }
                move(c.r(20), c.r(21), UINT32_MAX, 0x10282e60); return;
            }
        }
    }
    leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101e4e10, 0x10282e70); c.set(19, c.r(3));
    if (compare(c, std::int8_t(c.r(19))) >= 0) {
        c.set(20, c.u8(player + 0x10a)); const auto bound = std::int8_t(c.r(19)) * 5;
        if (compare(c, bound) != 0) {
            advance(0x10282eb8); c.set(3, 65535); compare(c, bound, 65535);
            c.set(3, 0xffff0001); compare(c, bound, -65535);
            const auto address = c.u32(cursor); const auto word = c.u16(address);
            const auto value = std::uint32_t(word) + c.u32(seed);
            leave(c, {{3, std::uint32_t(std::int32_t(std::int8_t(c.r(19))))}, {4, std::uint32_t(bound)}, {5, value}, {6, word}});
            compare(c, bound); c.set(21, std::uint32_t(std::int32_t(value) % bound));
        } else c.set(21, 0);
        advance(0x10283050); const auto value = sample(); const auto high = high_product(value, 0x55555556);
        leave(c, {{3, std::uint32_t(std::int32_t(value) % 3)}, {4, std::uint32_t(high)}, {5, value}});
        if (compare(c, std::int8_t(c.r(20)) + std::int32_t(c.r(21)), std::int32_t(c.r(3))) > 0) {
            c.put16(player + 0x19c, 0x6b); enter(c, 0x102830a8, {player, 0}); advance_row(c, player, 0); return;
        }
    }
    c.set(3, std::uint32_t(std::int32_t(actor.column())));
    bool already_there = compare(c, std::int32_t(c.r(3)), c.i8(frame.at(0x3b))) == 0;
    if (already_there) { c.set(3, std::uint32_t(std::int32_t(actor.row()))); already_there = compare(c, std::int32_t(c.r(3)), c.i8(frame.at(0x3a))) == 0; }
    if (already_there) {
        if (compare(c, std::int16_t(c.r(27)), 1) == 0 && compare(c, c.i8(player + 0x10d)) != 0) { same_position_move(0x10283118); return; }
        bool moving = compare(c, c.i8(player + 0x10d)) != 0;
        if (!moving) {
            leave(c, {{3, actor.match()}, {4, player}}); c.call(0x101f0dd0, 0x10283134); moving = compare(c, std::int8_t(c.r(3))) != 0;
        }
        if (moving) { same_position_move(0x10283160); return; }
        if (compare(c, c.i8(player + 0x10a)) != 0 && offer_move(4, 0x10283188, 0x102831c0, 0x6a, true)) return;
        if (compare(c, c.i8(player + 0x104)) != 0 && compare(c, c.r(27) & 4) == 0 && compare(c, c.r(27) & 2) == 0 &&
            compare(c, c.i8(player + 0x10a)) != 0 && offer_move(4, 0x10283204, 0x1028323c, 0x69, true)) return;
        same_position_move(0x10283264); return;
    }
    c.set(3, c.toc(-0x51e0)); c.set_f(1, c.f32(c.r(3) + 0x5c)); c.set(20, c.u8(player + 0x10c));
    c.put64(frame.at(0x64), fp.fctiwz(0, fp.fadds(c.f(1), c.f32(player + 0xe4)))); c.set(21, c.u32(frame.at(0x68)));
    const auto bound = std::int32_t(c.r(21));
    if (compare(c, bound) != 0) {
        advance(0x102832b8); c.set(3, 65535);
        bool narrow = compare(c, bound, 65535) <= 0;
        if (narrow) { c.set(3, 0xffff0001); narrow = compare(c, bound, -65535) >= 0; }
        if (narrow) { c.set(3, sample()); compare(c, bound); const auto residual = std::int32_t(std::int64_t(std::int32_t(c.r(3))) % bound); c.set(19, bound > 0 ? std::uint32_t(residual) : 0u - std::uint32_t(residual)); }
        else {
            c.put32(frame.at(0x64), 0x43300000); c.put32(frame.at(0x68), c.r(21) ^ 0x80000000);
            c.set_f(2, c.f64(doubles + 0x80)); c.set_f(1, fp.fsub(c.f64(frame.at(0x64)), c.f(2)));
            c.set_f(2, fp.fdiv(c.f(1), c.f64(doubles)));
            const auto word = c.u16(c.u32(cursor)); const auto value = std::uint32_t(word) + c.u32(seed);
            const auto residual = std::int32_t(value) % 65535;
            c.put32(frame.at(0x6c), 0x43300000); c.put32(frame.at(0x70), std::uint32_t(residual) ^ 0x80000000);
            c.set_f(1, c.f64(doubles + 0x80));
            c.put64(frame.at(0x74), fp.fctiwz(0, fp.fmul(c.f(2), fp.fsub(c.f64(frame.at(0x6c)), c.f(1)))));
            c.set(19, c.u32(frame.at(0x78)));
            const auto multiplied = c.r(21) * 10u, reduced = std::uint32_t(std::int32_t(multiplied) / 65535);
            const auto sign = shifted_sign(std::int32_t(std::uint32_t(high_product(value, std::int32_t(0x80008001))) + value), 15);
            shifted_sign(std::int32_t(std::uint32_t(high_product(multiplied, std::int32_t(0x80008001))) + multiplied), 15);
            leave(c, {{3, reduced}, {4, std::uint32_t(residual) ^ 0x80000000}, {5, 65535}, {6, sign}, {7, value}, {8, word}});
            enter(c, 0x10283390, {reduced}); sample_range(c, reduced);
            const auto numerator = c.r(3) + 5;
            c.set(4, shifted_sign(high_product(numerator, 0x66666667), 2)); c.set(3, std::uint32_t(std::int32_t(numerator) / 10)); c.set(19, c.r(19) + c.r(3));
            if (compare(c, std::int32_t(c.r(19)), std::int32_t(c.r(21) - 1)) > 0) c.set(19, c.r(21) - 1);
        }
    }
    else c.set(19, 0);
    if (compare(c, std::int8_t(c.r(20)), std::int32_t(c.r(19))) > 0) {
        if (compare(c, c.i8(player + 0x10a)) != 0 && offer_move(4, 0x10283424, 0x1028345c, 0x6a, true)) return;
        if (compare(c, c.i8(player + 0x104)) != 0 && compare(c, c.r(27) & 4) == 0 && compare(c, c.r(27) & 2) == 0 &&
            compare(c, c.i8(player + 0x10a)) != 0 && offer_move(4, 0x102834a0, 0x102834d8, 0x69, true)) return;
        c.put16(player + 0x19c, 0x68); move(c.u8(frame.at(0x3b)), c.u8(frame.at(0x3a)), UINT32_MAX, 0x10283500); return;
    }
    c.put16(player + 0x19c, 0x68); move(c.u8(frame.at(0x3b)), c.u8(frame.at(0x3a)), UINT32_MAX, 0x10283528);
}
}
