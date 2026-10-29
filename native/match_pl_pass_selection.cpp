#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"
#include "match_pl_pass_sampling.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void choose_pass_action(Cpu& c, std::uint32_t player, std::uint32_t mode, std::uint32_t force) {
    Frame frame(c, 0x10245870, 0xe0, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 30});
    const auto distances = c.toc(-0x6424), doubles = c.toc(-0x51e8), singles = c.toc(-0x51e0),
               seed = c.toc(-0x7b90), begin = c.toc(-0x7b8c), end = c.toc(-0x7b88), cursor = c.toc(-0x7b84);
    keep(c, {{30, player}, {17, distances}, {24, mode}, {14, doubles}, {13, force}, {29, seed},
             {26, 0}, {27, begin}, {19, 0}, {25, end}, {21, 0}, {31, cursor}, {22, 0}, {18, singles}, {23, 0}});
    const Player actor{c, player}; Fpu fp(c);
    const auto high_product = [](std::uint32_t value, std::int32_t multiplier) {
        return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * multiplier) >> 32);
    };
    const auto shifted_sign = [&](std::int32_t high, unsigned bits) {
        c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << bits) - 1)));
        return std::uint32_t(high < 0);
    };
    const auto step = [&](std::uint32_t back, auto publish) {
        const auto next = c.u32(cursor) + 2; c.set(3, next);
        const auto order = compare_logical(c, next, c.u32(end)); c.put32(cursor, next); publish();
        if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
    };
    const auto advance = [&](std::uint32_t back) { step(back, [] {}); };
    const auto word = [&] { return std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed); };
    const auto binary = [&] { const auto value = word(); c.set(3, value >> 31); return std::int32_t(value) % 2; };
    const auto ternary = [&](bool publish_residual = false) {
        const auto value = word(); c.set(4, value); c.set(3, std::uint32_t(high_product(value, 0x55555556)));
        const auto residual = std::int32_t(value) % 3; if (publish_residual) c.set(3, std::uint32_t(residual)); return residual;
    };
    bool outside = compare(c, c.i16(player + 0x19e), 50) < 0;
    if (!outside) outside = compare(c, c.i16(player + 0x19e), 310) > 0;
    if (outside) {
        if (compare(c, actor.side(), 1) == 0) {
            if (compare(c, c.i16(player + 0x19e), 310) > 0) c.set(21, 1); else c.set(19, 1);
        } else c.set(23, 1);
    }
    if (compare(c, std::int8_t(c.r(19))) == 0 && compare(c, c.i16(player + 0x19e), 40) > 0 && compare(c, c.i16(player + 0x19e), 140) < 0) c.set(19, 1);
    if (compare(c, c.i16(player + 0x19e), 130) > 0 && compare(c, c.i16(player + 0x19e), 230) < 0) {
        if (compare(c, std::int8_t(actor.side())) == 0) {
            if (compare(c, c.i16(player + 0x19e), 180) > 0) c.set(21, 1); else c.set(19, 1);
        } else c.set(22, 1);
    }
    if (compare(c, std::int8_t(c.r(21))) == 0 && compare(c, c.i16(player + 0x19e), 220) > 0 && compare(c, c.i16(player + 0x19e), 320) < 0) c.set(21, 1);
    const bool first = compare(c, std::int8_t(actor.side())) == 0;
    c.set(3, std::uint32_t(std::int32_t(actor.column()) - 4)); c.call(0x10775540, first ? 0x102459b0 : 0x102459f0);
    c.set(15, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
    c.set(3, std::uint32_t(std::int32_t(actor.row()) - (first ? 11 : 0))); c.call(0x10775540, first ? 0x102459c4 : 0x10245a04);
    c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(3))))) ; c.set_f(1, c.f32(distances + c.r(3) * 36 + c.r(15) * 4));
    c.set_f(0, c.f64(doubles + 0x18)); fp.fcmp(0, c.f(1), c.f(0)); bool cross = (c.cr(0) & 8) != 0;
    if (cross) { enter(c, 0x10245a30, {player}); in_wide_column(c, player); cross = compare(c, std::int8_t(c.r(3))) != 0; }
    if (cross) cross = compare(c, std::int8_t(c.r(24)), 4) != 0;
    if (cross) {
        c.set(15, 94); const bool first_side = compare(c, std::int8_t(actor.side())) == 0;
        advance(first_side ? 0x10245a74 : 0x10245ac4); const auto offset = binary();
        if (first_side) c.s.wr<1>(reg::xer_ca, std::uint32_t(offset) <= 11);
        c.set(28, std::uint32_t(std::int32_t(std::int8_t(first_side ? 11 - offset : offset))));
        advance(0x10245b10); c.set(20, std::uint32_t(std::int32_t(std::int8_t(ternary(true) + 3))));
    } else {
        enter(c, 0x10245b58, {player}); in_own_half(c, player); const bool own_half = compare(c, std::int8_t(c.r(3))) != 0;
        const bool short_pass = compare(c, std::int8_t(c.r(24)), 4) == 0;
        struct Style { unsigned range, triangle; int row_bias, column_bias; bool forward; std::array<std::uint32_t, 8> backs; } style;
        if (!own_half && short_pass) {
            c.set(15, 92); style = {2, 2, 1, 1, false, {0x10245ba0,0x10245c08,0x10245c68,0x10245cb0,0x10245d14,0x10245d7c,0x10245ddc,0x10245e24}};
        } else if (!own_half) {
            c.set(15, 93); style = {4, 3, 2, 2, true, {0x10245e90,0x10245efc,0x10245f60,0x10245fb4,0x10246024,0x10246090,0x102460f4,0x10246148}};
        } else if (short_pass) {
            c.set(15, 91); style = {2, 2, 2, 2, false, {0x102461f4,0x1024625c,0x102462bc,0x10246304,0x10246368,0x102463d0,0x10246430,0x10246478}};
        } else {
            c.set(4, actor.match()); c.set(3, 9); if (compare(c, c.i8(c.r(4) + 0x791f), -1) == 0) c.set(3, 62); c.set(15, c.r(3));
            style = {5, 3, 4, 3, true, {0x10246500,0x10246578,0x102465e8,0x1024663c,0x102466ac,0x10246724,0x10246794,0x102467e8}};
        }
        const auto coordinate = [&](bool horizontal) {
            const auto negative = horizontal ? 21u : 22u, positive = horizontal ? 19u : 23u;
            const auto field = horizontal ? 0x791du : 0x791eu, output = horizontal ? 20u : 28u, index = horizontal ? 4u : 0u;
            const auto bias = horizontal ? style.column_bias : style.row_bias;
            const bool lower = compare(c, std::int8_t(c.r(negative))) != 0;
            const bool upper = !lower && compare(c, std::int8_t(c.r(positive))) != 0;
            c.set(4, actor.match());
            if (lower || upper) {
                c.set(16, c.u8(c.r(4) + field)); advance(style.backs[index + unsigned(!lower)]);
                std::int32_t residual;
                if (style.range == 2) residual = binary();
                else if (style.range == 4) { const auto value = word(); c.set(3, value >> 31); residual = std::int32_t(value) % 4; }
                else {
                    const auto value = word(); c.set(4, value); const auto high = high_product(value, 0x66666667);
                    c.set(3, shifted_sign(high, 1)); residual = std::int32_t(value) % 5;
                }
                c.set(3, lower ? c.r(16) - std::uint32_t(residual) : std::uint32_t(residual));
                const auto target = lower ? c.r(3) - unsigned(bias) : c.r(16) + c.r(3) + unsigned(bias);
                c.set(output, std::uint32_t(std::int32_t(std::int8_t(target)))); return;
            }
            const unsigned base_register = style.triangle == 2 || (!horizontal && style.range == 5) ? 16 : 20;
            c.set(base_register, c.u8(c.r(4) + field)); advance(style.backs[index + 2]);
            if (style.triangle == 2) {
                c.set(4, c.u32(cursor)); c.set(3, c.u32(cursor)); c.set(5, c.u16(c.r(4))); c.set(4, c.u32(seed)); c.set(5, c.r(5) + c.r(4));
                const auto value = c.r(5); step(style.backs[index + 3], [&] { c.set(4, value >> 31); c.set(20, std::uint32_t(std::int32_t(value) % 2)); });
                const auto other = binary(); c.set(output, std::uint32_t(std::int32_t(std::int8_t(c.r(16) + c.r(20) - std::uint32_t(other)))));
            } else {
                c.set(5, c.u32(cursor)); c.set(3, c.u32(cursor)); c.set(6, c.u16(c.r(5))); c.set(5, c.u32(seed)); c.set(5, c.r(6) + c.r(5));
                const auto value = c.r(5); step(style.backs[index + 3], [&] { c.set(4, std::uint32_t(high_product(value, 0x55555556))); c.set(base_register == 16 ? 20 : 16, std::uint32_t(std::int32_t(value) % 3)); });
                const auto other = ternary(); c.set(output, std::uint32_t(std::int32_t(std::int8_t(c.r(20) + c.r(16) - std::uint32_t(other)))));
            }
        };
        coordinate(false); coordinate(true);
        if (style.forward) {
            if (compare(c, std::int8_t(actor.side())) == 0) { if (compare(c, std::int8_t(c.r(23))) == 0) c.set(28, c.r(28) + 2); }
            else if (compare(c, std::int8_t(c.r(22))) == 0) c.set(28, c.r(28) - 2);
        }
    }
    c.set(16, 0);
    if (compare(c, std::int8_t(c.r(24)), -1) != 0) {
        if (compare(c, std::int8_t(c.r(13))) != 0) c.set(16, 1);
        else {
            bool refine = compare(c, c.i8(player + 0x10e)) != 0;
            if (!refine) { advance(0x1024689c); refine = compare(c, ternary()) != 0; }
            if (refine) {
                bool excluded = compare(c, std::int8_t(actor.side())) == 0 && compare(c, actor.row(), 2) < 0;
                if (!excluded) excluded = compare(c, actor.side(), 1) == 0 && compare(c, actor.row(), 9) > 0;
                if (!excluded) c.set(16, 1);
            }
        }
    }
    if (compare(c, std::int8_t(c.r(16))) != 0) {
        const bool first_side = compare(c, std::int8_t(actor.side())) == 0;
        std::uint32_t target_column;
        if (compare(c, std::int8_t(c.r(21))) != 0) target_column = 1;
        else if (compare(c, std::int8_t(c.r(19))) != 0) target_column = 7;
        else { advance(first_side ? 0x1024696c : 0x10246a3c); target_column = compare(c, binary()) != 0 ? 1 : 7; }
        c.set(3, first_side ? 10 : 1);
        for (;;) {
            const auto source_row = actor.row(), target_row = std::int8_t(c.r(3));
            if (first_side) { c.set(5, c.u8(player + 0x106)); c.set(4, std::uint32_t(target_row - source_row)); c.set(5, std::uint32_t(std::int32_t(source_row))); }
            else { c.set(4, std::uint32_t(source_row - target_row)); c.set(5, std::uint32_t(std::int32_t(target_row))); }
            if (compare(c, std::int32_t(c.r(4)), 5) > 0) { c.set(3, c.r(3) + (first_side ? UINT32_MAX : 1)); continue; }
            c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(24))))); if (compare(c, std::int32_t(c.r(4)), 4) != 0) break;
            if (first_side) { c.set(5, std::uint32_t(std::int32_t(actor.row()))); c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(3))) - std::int32_t(c.r(5)))); }
            else { c.set(5, std::uint32_t(std::int32_t(std::int8_t(c.r(3))))); c.set(4, std::uint32_t(std::int32_t(actor.row())) - c.r(5)); }
            if (compare(c, std::int32_t(c.r(4)), 3) <= 0) break;
            c.set(3, c.r(3) + (first_side ? UINT32_MAX : 1));
        }
        c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(20))))); const auto x = c.r(4) + target_column;
        c.set(6, x); c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(28))))); c.set(3, c.r(4) + std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
        c.set(5, x >> 31); const auto x_sum = x + c.r(5), y_sum = c.r(3) + (c.r(3) >> 31);
        c.set(4, std::uint32_t(std::int32_t(x_sum) >> 1)); c.s.wr<1>(reg::xer_ca, std::int32_t(y_sum) < 0 && (y_sum & 1));
        c.set(20, std::uint32_t(std::int32_t(std::int8_t(c.r(4))))); c.set(28, std::uint32_t(std::int32_t(std::int8_t(std::int32_t(y_sum) >> 1))));
    }
    c.set(4, c.u8(player + 0x2c) + 1); c.put8(player + 0x2c, c.r(4));
    leave(c, {{3, player}, {6, std::uint32_t(std::int32_t(std::int8_t(c.r(15))))}, {7, frame.at(0x40)}, {8, 0}, {9, c.r(20)}, {10, c.r(28)}});
    c.put8(frame.at(0x40), 1); leave(c, {{4, c.u8(player + 0x105)}, {5, c.u8(player + 0x106)}}); c.call(0x10247030, 0x10246b30);
    if (compare(c, std::int8_t(c.r(3))) == 0) return;
    if (compare(c, std::int16_t(c.r(15)), 54) != 0) {
        leave(c, {{3, actor.match()}, {4, actor.side()}, {5, c.u8(player + 0x106)}, {6, c.r(28)}, {7, 0}}); c.call(0x101f5230, 0x10246b5c); c.set(26, c.r(3));
    }
    if (compare_logical(c, c.r(26)) != 0) {
        const auto goal_distance = [&](unsigned destination, bool second_read) {
            const Player defender{c, c.r(26)}; const bool first_goal = compare(c, std::int8_t(defender.side())) == 0;
            c.set(3, std::uint32_t(std::int32_t(defender.column()) - 4)); c.call(0x10775540, second_read ? first_goal ? 0x10246c0c : 0x10246c4c : first_goal ? 0x10246b84 : 0x10246bc4);
            c.set(13, std::uint32_t(std::int32_t(std::int8_t(c.r(3))))); c.set(3, std::uint32_t(std::int32_t(defender.row()) - (first_goal ? 11 : 0)));
            c.call(0x10775540, second_read ? first_goal ? 0x10246c20 : 0x10246c60 : first_goal ? 0x10246b98 : 0x10246bd8);
            c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))) * 36); c.set_f(destination, c.f32(distances + c.r(3) + std::uint32_t(std::int32_t(std::int8_t(c.r(13)))) * 4));
        };
        goal_distance(31, false); goal_distance(30, true);
        sample_interception_distance(c, frame, 13, 0x44, singles, doubles, seed, begin, end, cursor, 0x10246cb8, 0x10246df0);
        if (compare(c, std::int32_t(c.r(13))) == 0) {
            c.put32(frame.at(0x38), UINT32_MAX); c.put32(frame.at(0x3c), UINT32_MAX);
            leave(c, {{3, actor.match()}, {4, UINT32_MAX}, {5, c.r(26)}, {6, 0}, {7, 0}, {8, 8}, {9, c.u8(c.r(26) + 0x105)}, {10, c.u8(c.r(26) + 0x106)}}); c.call(0x101d68f0, 0x10246ef0);
        } else {
            compare_logical(c, c.r(26));
            leave(c, {{3, c.u32(actor.match() + 0x79f4)}, {4, 0x1f78}, {5, actor.match()}, {6, c.u8(c.r(26) + 0x2f)}, {7, actor.side()}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}}); c.call(0x10201180, 0x10246f24);
            c.set(4, actor.side()); c.set(3, 7); c.set(5, actor.match()); c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(4))) * 54)); c.set(5, c.r(5) + 0x10000 + c.r(4));
            c.set(4, c.u8(c.r(5) - 0x1fb6)); c.put8(c.r(5) - 0x1fb6, c.r(4) + 1); c.set(4, c.u8(c.r(26) + 0x13)); c.put8(c.r(26) + 0x13, c.r(4) + 1);
            c.set(4, c.u8(player + 1)); c.put8(player + 1, c.r(4) - 1); c.set(5, c.u8(c.r(26) + 0x105)); c.set(4, actor.match()); c.put8(c.r(4) + 0x791d, c.r(5));
            c.set(5, c.u8(c.r(26) + 0x106)); c.set(4, actor.match()); c.put8(c.r(4) + 0x791e, c.r(5)); c.set(4, actor.match()); c.put8(c.r(4) + 0x7928, c.r(3));
            c.set(3, unsigned(std::int8_t(actor.side()) == 0)); c.set(4, actor.match()); c.put8(c.r(4) + 0x7924, c.r(3)); c.set(3, actor.match() + 0x10000); c.put32(c.r(3) - 0x2000, 0); return;
        }
    }
    c.set(3, actor.match()); if (compare(c, c.i8(c.r(3) + 0x7928)) != 0) return;
    c.set(3, actor.match()); bool outside_pitch = compare(c, c.i8(c.r(3) + 0x791d)) < 0;
    if (!outside_pitch) { c.set(3, actor.match()); outside_pitch = compare(c, c.i8(c.r(3) + 0x791e)) < 0; }
    if (!outside_pitch) { c.set(3, actor.match()); outside_pitch = compare(c, c.i8(c.r(3) + 0x791d), 9) >= 0; }
    if (!outside_pitch) { c.set(3, actor.match()); outside_pitch = compare(c, c.i8(c.r(3) + 0x791e), 12) >= 0; }
    if (outside_pitch) { leave(c, {{3, actor.match()}, {4, actor.side()}}); c.call(0x101f0c00, 0x10247014); }
}
}
