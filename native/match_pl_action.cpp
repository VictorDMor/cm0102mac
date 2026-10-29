#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void dispatch_action(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x102775e0, 0x90, {.first_gpr = 23, .gpr_offset = -0x34, .first_fpr = 31});
    const auto distances = c.toc(-0x6424), doubles = c.toc(-0x51e8), seed = c.toc(-0x7b90),
               begin = c.toc(-0x7b8c), end = c.toc(-0x7b88), cursor = c.toc(-0x7b84);
    keep(c, {{25, distances}, {26, doubles}, {27, end}, {28, begin}, {29, seed}, {30, cursor}, {31, player}});
    const Player actor{c, player};
    Fpu fp(c);
    const auto high_product = [](std::uint32_t value, std::int32_t multiplier) {
        return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * multiplier) >> 32);
    };
    const auto shifted_sign = [&](std::int32_t high, unsigned bits) {
        c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << bits) - 1)));
        return std::uint32_t(high < 0);
    };
    const auto advance = [&](std::uint32_t back, unsigned second_trait = 0, bool load_intensity = false, bool load_status = false) {
        const auto next = c.u32(cursor) + 2; c.set(3, next);
        if (load_intensity) c.set_f(31, c.f32(player + 0x50));
        if (second_trait) { c.set(25, c.u8(player + 0x107)); c.set(second_trait, c.u8(player + 0x10a)); }
        if (load_status) c.set(24, c.u8(player + 0x110));
        const auto order = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
        if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
    };
    const auto sample = [&] { return std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed); };
    const auto main_actor = [&] {
        c.set(3, actor.match() + 0x10000);
        return compare_logical(c, player, c.u32(c.r(3) - 0x2000)) == 0;
    };
    const auto field_role = [&](std::uint32_t back) {
        enter(c, back, {player}); current_role(c, player);
        return compare(c, c.r(3) & 1) == 0;
    };
    const auto visit_peers = [&](std::uint32_t query_back, std::uint32_t random_back, std::uint32_t decision_back, unsigned second_trait) {
        Kept<24> index(c, 0);
        for (;;) {
            c.set(3, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(index)))));
            if (compare(c, std::int8_t(std::uint32_t(index)), c.i8(player + 0x104)) >= 0) break;
            const auto offset = std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(index)))) * 4;
            leave(c, {{3, actor.match()}, {4, c.u32(player + 0x1b4 + offset)},
                      {5, c.u8(player + 0x105)}, {6, c.u8(player + 0x106)}});
            c.call(0x101d8120, query_back);
            if (compare(c, std::int8_t(c.r(3))) != 0) {
                advance(random_back, second_trait);
                const auto word = c.u16(c.u32(cursor));
                const auto value = std::uint32_t(word) + c.u32(seed);
                const auto high = high_product(value, std::int32_t(0x91a2b3c5));
                const auto partial = std::int32_t(std::uint32_t(high) + value);
                const auto sign = shifted_sign(partial, 9), residual = std::uint32_t(std::int32_t(value) % 900);
                const auto traits = std::int8_t(c.r(25)) * std::int8_t(c.r(second_trait));
                leave(c, {{3, std::uint32_t(traits)}, {4, residual}, {5, sign}, {6, value}});
                if (compare(c, traits, std::int32_t(residual) + 50) <= 0) {
                    const auto peer = c.u32(player + 0x1b4 + offset);
                    enter(c, decision_back, {peer}); choose_context_action(c, peer);
                }
            }
            index = std::uint32_t(index) + 1;
        }
    };
    const auto context_selects_secondary = [&](std::uint32_t back) {
        const auto side_word = actor.side(); c.set(4, side_word); c.set(24, actor.match());
        const auto match = c.r(24), opposing = unsigned(std::int8_t(side_word) == 0);
        c.set(3, extended(std::int16_t(c.i8(match + 0x7924))));
        bool selected;
        if (compare(c, std::int32_t(c.r(3)), opposing) == 0) selected = false;
        else {
            c.set(3, std::uint32_t(std::int32_t(std::int8_t(side_word))));
            if (compare(c, std::int32_t(c.r(3)), c.i8(match + 0x7924)) == 0) selected = true;
            else {
                c.set(3, match + 0x10000); const auto other = c.u32(c.r(3) - 0x2008);
                bool same_side = false;
                if (compare_logical(c, other) != 0) {
                    c.set(3, std::uint32_t(std::int32_t(c.i8(other + 0x27))));
                    same_side = compare(c, std::int32_t(c.r(3)), std::int8_t(side_word)) == 0;
                }
                if (same_side) selected = true;
                else {
                    leave(c, {{3, match}, {4, opposing}, {5, c.u8(match + 0x791d)}, {6, c.u8(match + 0x791e)}});
                    c.call(0x101eea30, back);
                    selected = compare(c, std::int8_t(c.r(3))) != 0;
                    if (selected) selected = compare(c, c.i8(c.r(24) + 0x791f), -1) != 0;
                }
            }
        }
        compare(c, selected); return selected;
    };

    if (!main_actor() && field_role(0x1027762c)) {
        advance(0x10277658, 0, true);
        const auto word = c.u16(c.u32(cursor));
        const auto value = std::uint32_t(word) + c.u32(seed);
        // The source advances the second cursor before finishing the first
        // quotient. Its reseed callback sees R3=the quotient sign, not cursor.
        const auto next = c.u32(cursor) + 2;
        const auto order = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
        const auto high = high_product(value, 0x7e07e07f);
        const auto sign = shifted_sign(high, 5);
        c.set(24, std::uint32_t(std::int32_t(value) % 65));
        leave(c, {{3, sign}, {4, std::uint32_t(high)}, {5, value}, {6, word}});
        if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, 0x102776b0); c.put32(seed, c.r(3) & 0xffff); }
        const auto second_word = c.u16(c.u32(cursor));
        const auto second = std::uint32_t(second_word) + c.u32(seed);
        c.put32(frame.at(0x40), 0x43300000); c.put32(frame.at(0x48), 0x43300000);
        c.put32(frame.at(0x4c), c.r(24) ^ 0x80000000);
        c.set_f(3, fp.fsub(c.f64(frame.at(0x48)), c.f64(doubles + 0x80)));
        c.set_f(1, c.f64(doubles + 0x80)); c.set_f(2, c.f64(doubles + 0x378));
        c.set(3, shifted_sign(high_product(second, 0x66666667), 2)); c.set(5, 0x66666667); c.set(6, second); c.set(7, second_word);
        c.set(4, c.r(24) ^ 0x80000000);
        c.put32(frame.at(0x44), std::uint32_t(std::int32_t(second) % 10) ^ 0x80000000);
        c.set_f(0, fp.fsub(c.f64(frame.at(0x40)), c.f(1)));
        c.set_f(0, fp.multiply_add(0x10277724, c.f(31), c.f(2), c.f(0)));
        fp.fcmp(0, c.f(3), c.f(0)); const auto bits = c.cr(0);
        c.set_cr(0, (bits & ~2u) | ((bits & 6) ? 2 : 0));
        if (c.cr(0) & 2) {
            visit_peers(0x1027775c, 0x1027778c, 0x102777f4, 26); c.put16(player + 0x19c, 103); return;
        }
    }
    if (!main_actor() && field_role(0x10277834) && !context_selects_secondary(0x102778d8)) {
        advance(0x10277924); const auto value = sample();
        leave(c, {{3, shifted_sign(high_product(value, 0x66666667), 3)}, {4, value}});
        if (compare(c, std::int32_t(value) % 20) == 0 && compare(c, c.i8(player + 0x110)) == 0) {
            visit_peers(0x10277994, 0x102779c4, 0x10277a2c, 26);
            c.set(4, extended(c.i16(player + 0x36))); c.put16(player + 0x36, c.r(4) - 15);
            enter(c, 0x10277a58, {player}); improve_condition(c, player); return;
        }
    }
    if (!main_actor()) {
        c.set(3, actor.match() + std::uint32_t(std::int32_t(std::int8_t(actor.side()))));
        if (compare(c, c.u8(c.r(3) + 0x1c4), 5) == 0 && field_role(0x10277a94)) {
            const bool second = compare(c, std::int8_t(actor.side())) != 0;
            c.set(3, std::uint32_t(std::int32_t(actor.column()) - 4)); c.call(0x10775540, second ? 0x10277af8 : 0x10277ab8);
            c.set(24, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
            c.set(3, std::uint32_t(std::int32_t(actor.row()) - (second ? 0 : 11))); c.call(0x10775540, second ? 0x10277b0c : 0x10277acc);
            const auto offset = std::uint32_t(std::int8_t(c.r(3)) * 36) + (std::uint32_t(std::int32_t(std::int8_t(c.r(24)))) << 2);
            c.set_f(31, c.f32(distances + offset)); advance(0x10277b44);
            auto value = sample(); shifted_sign(high_product(value, 0x2aaaaaab), 1);
            leave(c, {{3, std::uint32_t(std::int32_t(value) < 0)}, {4, value}, {5, value - c.u32(seed)}});
            c.put32(frame.at(0x40), 0x43300000); c.put32(frame.at(0x44), std::uint32_t(std::int32_t(value) % 12) ^ 0x80000000);
            c.set_f(1, c.f64(doubles + 0x80)); c.set_f(0, fp.fsubs(c.f64(frame.at(0x40)), c.f(1)));
            fp.fcmp(0, c.f(31), c.f(0));
            if (c.cr(0) & 8) {
                advance(0x10277bc4, 0, false, true); value = sample();
                leave(c, {{3, std::uint32_t(std::int32_t(value) % 10)},
                          {4, shifted_sign(high_product(value, 0x66666667), 2)}, {5, value}});
                if (compare(c, std::int8_t(c.r(24)), std::int32_t(c.r(3))) < 0) {
                    visit_peers(0x10277c30, 0x10277c60, 0x10277cc8, 26);
                    enter(c, 0x10277ce8, {player}); improve_condition(c, player); return;
                }
            }
        }
    }
    if (compare(c, c.i8(player + 0x2c)) != 0 && !main_actor()) {
        visit_peers(0x10277d34, 0x10277d64, 0x10277dcc, 26); c.put16(player + 0x19c, 103); return;
    }
    if (main_actor()) {
        visit_peers(0x10277e2c, 0x10277e5c, 0x10277ec4, 23);
        enter(c, 0x10277ee8, {player, 0}); choose_primary_action(c, player, 0);
        c.set(3, actor.match()); c.put8(c.r(3) + 0x1c0, 0);
        if (main_actor() || compare(c, c.i16(player + 0x19c), 109) == 0 || compare(c, c.i8(player + 0x2c)) != 0) return;
        c.set(3, actor.match()); if (compare(c, c.i8(c.r(3) + 0x7928)) != 0 || compare(c, c.i8(player + 0x10a)) == 0) return;
        c.put64(frame.at(0x40), fp.fctiwz(0, c.f32(player + 0x80)));
        const auto bound_word = c.u32(frame.at(0x44)); const auto bound = std::int32_t(bound_word);
        c.set(25, bound_word); std::uint32_t draw = 0;
        if (compare(c, bound) != 0) {
            advance(0x10277f7c);
            if (compare(c, bound, 65535) <= 0 && compare(c, bound, -65535) >= 0) {
                c.set(3, sample()); compare(c, bound);
                const auto residual = std::int32_t(std::int64_t(std::int32_t(c.r(3))) % bound);
                draw = bound > 0 ? std::uint32_t(residual) : 0u - std::uint32_t(residual);
            } else {
                c.put32(frame.at(0x40), 0x43300000); c.put32(frame.at(0x44), bound_word ^ 0x80000000);
                c.set_f(2, c.f64(doubles + 0x80)); c.set_f(1, fp.fsub(c.f64(frame.at(0x40)), c.f(2)));
                c.set_f(2, fp.fdiv(c.f(1), c.f64(doubles)));
                const auto word = c.u16(c.u32(cursor));
                const auto value = std::uint32_t(word) + c.u32(seed);
                const auto residual = std::int32_t(value) % 65535;
                c.put32(frame.at(0x48), 0x43300000); c.put32(frame.at(0x4c), std::uint32_t(residual) ^ 0x80000000);
                c.set_f(1, c.f64(doubles + 0x80));
                c.put64(frame.at(0x38), fp.fctiwz(0, fp.fmul(c.f(2), fp.fsub(c.f64(frame.at(0x48)), c.f(1)))));
                const auto whole = c.u32(frame.at(0x3c)); c.set(24, whole);
                const auto multiplied = bound_word * 10u, reduced = std::uint32_t(std::int32_t(multiplied) / 65535);
                shifted_sign(std::int32_t(std::uint32_t(high_product(multiplied, std::int32_t(0x80008001))) + multiplied), 15);
                leave(c, {{3, reduced}, {4, std::uint32_t(residual) ^ 0x80000000}, {5, 65535},
                          {6, std::uint32_t(std::int32_t(value) < 0)}, {7, value}, {8, word}});
                enter(c, 0x10278054, {reduced}); sample_range(c, reduced);
                const auto numerator = std::int32_t(c.r(3) + 5);
                c.set(4, shifted_sign(high_product(std::uint32_t(numerator), 0x66666667), 2)); c.set(3, std::uint32_t(numerator / 10));
                draw = whole + c.r(3);
                if (compare(c, std::int32_t(draw), std::int32_t(bound_word - 1)) > 0) draw = bound_word - 1;
            }
        }
        c.set(24, draw); advance(0x102780d8); const auto value = sample();
        leave(c, {{3, shifted_sign(high_product(value, 0x66666667), 1)}, {4, value}});
        if (compare(c, std::int32_t(draw), std::int32_t(value) % 5) > 0) {
            enter(c, 0x10278120, {player}); dispatch_action(c, player);
        }
        return;
    }
    if (context_selects_secondary(0x102781c0)) { enter(c, 0x102781f8, {player, 1}); choose_secondary_action(c, player); }
    else { enter(c, 0x10278204, {player}); choose_context_action(c, player); }
}
}
