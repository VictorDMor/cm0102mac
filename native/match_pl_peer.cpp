#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;

std::uint32_t resolve_peer_contact(Cpu& c, std::uint32_t player, std::uint32_t depth_word) {
    Frame frame(c, 0x102876c0, 0x90, {.first_gpr = 18});
    const auto doubles = c.toc(-0x51e8), singles = c.toc(-0x51e0), seed = c.toc(-0x7b90),
               begin = c.toc(-0x7b8c), end = c.toc(-0x7b88), cursor = c.toc(-0x7b84);
    keep(c, {{18, depth_word}, {19, end}, {20, begin}, {22, UINT32_MAX}, {23, 0}, {24, singles},
             {26, doubles}, {28, seed}, {29, cursor}, {30, 0}, {31, player}});
    c.put8(frame.at(0x38), 0);
    const Player actor{c, player};
    const auto cell = actor.match() + 0xa08 + std::uint32_t(actor.column() * 0x450 + actor.row() * 0x5c);
    keep(c, {{21, cell}});
    const auto advance = [&](std::uint32_t back) {
        const auto next = c.u32(cursor) + 2; c.set(3, next);
        const auto order = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
        if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
    };
    const auto sample = [&] { return std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed); };
    const auto shifted_sign = [&](std::int32_t high, unsigned bits) {
        c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << bits) - 1)));
        return std::uint32_t(high < 0);
    };
    const auto high_product = [](std::uint32_t value, std::int32_t multiplier) {
        return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * multiplier) >> 32);
    };
    Fpu fp(c);
    const auto convert = [&](std::int32_t value, unsigned at, bool single) {
        c.put32(frame.at(at), 0x43300000); c.put32(frame.at(at + 4), std::uint32_t(value) ^ 0x80000000);
        return single ? fp.fsubs(c.f64(frame.at(at)), c.f64(doubles + 0x80)) :
                        fp.fsub(c.f64(frame.at(at)), c.f64(doubles + 0x80));
    };
    const auto respond = [&](std::uint32_t peer, unsigned kind, std::uint32_t back) {
        enter(c, back, {player, kind, peer}); accepts_response(c, player, kind, peer);
        return compare(c, std::int8_t(c.r(3))) != 0;
    };
    // The four contact policies share the same intensity update, but publish
    // different conversion slots and use two distinct pairs of coefficients.
    const auto affect_peer = [&](std::uint32_t peer, unsigned scale_slot, unsigned floor_slot,
                                 unsigned denominator, unsigned intercept) {
        c.set(4, c.u32(peer + 0x6c));
        c.set_f(3, convert(c.i8(c.r(4) + 0x69), scale_slot, false));
        c.set_f(2, c.f64(doubles + denominator)); c.set_f(4, c.f64(doubles + intercept));
        c.set_f(1, c.f64(doubles + 0x80)); c.put32(frame.at(0x44), 0x43300000);
        const auto factor = fp.frsp(fp.fsub(c.f(4), fp.fdiv(c.f(3), c.f(2)))); c.set_f(2, factor);
        c.put_f32(peer + 0x1a8, float(fp.fmuls(c.f32(peer + 0x1a8), factor)));
        c.set(3, c.u32(peer + 0x6c)); c.set_f(2, c.f32(peer + 0x1a8));
        auto floor_word = 50u - std::uint32_t(std::int32_t(c.i8(c.r(3) + 0x69))) * 2u;
        c.s.wr<1>(reg::xer_ca, std::uint32_t(std::int32_t(c.i8(c.r(3) + 0x69))) * 2u <= 50);
        c.set_f(0, convert(std::int32_t(floor_word), 0x44, true)); fp.fcmp(0, c.f(2), c.f(0));
        double value;
        if (c.cr(0) & 4) value = c.f32(peer + 0x1a8);
        else {
            c.set(3, c.u32(peer + 0x6c));
            floor_word = 50u - std::uint32_t(std::int32_t(c.i8(c.r(3) + 0x69))) * 2u;
            c.s.wr<1>(reg::xer_ca, std::uint32_t(std::int32_t(c.i8(c.r(3) + 0x69))) * 2u <= 50);
            c.set_f(1, c.f64(doubles + 0x80)); value = convert(std::int32_t(floor_word), floor_slot, true);
        }
        c.put_f32(peer + 0x1a8, float(value));
        c.set_f(0, c.f64(doubles + 0x3a8)); c.set_f(1, c.f32(peer + 0x1a8)); fp.fcmp(0, c.f(1), c.f(0));
        if (c.cr(0) & 4) c.put_f32(peer + 0x1a8, c.f32(singles + 0x1c));
    };
    const auto team_effect = [&](std::uint32_t peer, unsigned amount, std::uint32_t back) {
        leave(c, {{3, actor.match()}, {4, c.u8(peer + 0x27)}}); c.set_f(1, c.f32(singles + amount));
        c.call(0x101d6860, back);
    };
    for (std::uint32_t index = 0;; ++index) {
        c.set(23, index);
        const auto opposing = std::uint32_t(actor.side() == 0);
        leave(c, {{3, cell + opposing}, {4, std::uint32_t(std::int32_t(std::int8_t(index)))}});
        if (compare(c, std::int8_t(index), c.i8(cell + 0x58 + opposing)) >= 0) break;
        if (compare(c, std::int16_t(c.r(22)), -1) != 0) break;
        const auto peer = c.u32(cell + opposing * 0x2c + std::uint32_t(std::int8_t(index)) * 4);
        keep(c, {{25, peer}});
        if (compare_logical(c, peer) == 0 || compare_logical(c, c.u32(peer + 0x6c)) == 0 ||
            compare_logical(c, c.u32(peer + 0x70)) == 0) continue;
        if (respond(peer, 4, 0x10287788)) {
            keep(c, {{22, 0x20e3}, {27, 0x31}});
            team_effect(peer, 0x110, 0x102877a8); affect_peer(peer, 0x4c, 0x3c, 0x3d0, 0xa8);
            c.set(30, peer);
        } else if (respond(peer, 3, 0x10287898)) {
            const auto staff = c.u32(player + 0x6c), organization = c.u32(staff + 0xc);
            const auto organization_trait = std::int32_t(c.i8(organization + 0x119));
            const auto difference = organization_trait - 8;
            leave(c, {{4, std::uint32_t(difference)}, {5, std::uint32_t(organization_trait)}});
            c.s.wr<1>(reg::xer_ca, difference != 0); // srawi31 of difference|-difference
            const auto penalty = difference == 0 ? 0u : 700u, bound = 1000 - penalty;
            c.set(22, penalty); c.s.wr<1>(reg::xer_ca, penalty <= 1000); compare(c, bound);
            // The penalty is exactly0 or700, hence the bound is300 or1000.
            advance(0x10287904); c.s.wr<1>(reg::xer_ca, penalty <= 1000);
            compare(c, bound, 65535); c.s.wr<1>(reg::xer_ca, penalty <= 1000); compare(c, bound, -65535);
            const auto value = sample(); compare(c, bound);
            leave(c, {{3, bound}, {4, value}, {5, value - c.u32(seed)}});
            c.set(27, std::uint32_t(std::int32_t(value) % std::int32_t(bound)));
            if (compare(c, std::int32_t(c.r(27)), 10) < 0) {
                keep(c, {{22, 0x2108}, {27, 0x61}});
                team_effect(peer, 0x40, 0x10287a80); affect_peer(peer, 0x3c, 0x4c, 0x3d0, 0xa8);
            } else {
                advance(0x10287b7c); const auto sum = sample(); const auto high = high_product(sum, 0x66666667);
                leave(c, {{3, shifted_sign(high, 2)}, {4, sum}});
                const auto selection = std::int32_t(sum) % 10;
                if (compare(c, selection, 4) < 0) {
                    if (compare(c, selection) >= 0) keep(c, {{22, 0x20e4}, {27, 0x32}});
                    else keep(c, {{22, 0x20e5}, {27, 0x2f}});
                } else if (compare(c, selection, 9) < 0) keep(c, {{22, 0x20e1}, {27, 0x30}});
                else keep(c, {{22, 0x20e5}, {27, 0x2f}});
                team_effect(peer, 0x2c, 0x10287c04); affect_peer(peer, 0x3c, 0x4c, 0x3d0, 0xa8);
            }
            c.set(30, peer);
        } else if (respond(peer, 2, 0x10287cf4)) {
            advance(0x10287d1c); const auto value = sample(); c.set(3, value >> 31);
            if (compare(c, std::int32_t(value) % 2) == 0) {
                if (compare(c, c.i16(player + 0x19c), 95) != 0) keep(c, {{22, 0x210b}, {27, 0x5f}});
                else keep(c, {{22, 0x20e2}, {27, 0x60}});
            } else keep(c, {{22, 0x20e2}, {27, 0x60}});
            c.set(30, peer); team_effect(peer, 0xa4, 0x10287d8c); affect_peer(peer, 0x3c, 0x4c, 0xb8, 0x380);
        }
    }
    const auto selected = c.r(30);
    if (compare_logical(c, selected) == 0) return result(c, 0);
    const auto depth = std::int8_t(c.r(18)); compare(c, depth, 1); c.put16(player + 0x19c, extended(std::int8_t(c.r(27))));
    bool suppress = false;
    if (depth > 1) {
        advance(0x10287edc); auto value = sample(); auto high = high_product(value, 0x66666667);
        leave(c, {{3, shifted_sign(high, 1)}, {4, value}});
        if (compare(c, std::int32_t(value) % 5) != 0) {
            advance(0x10287f38); value = sample(); high = high_product(value, 0x55555556);
            const auto count = c.i8(actor.match() + 0x1bc); const auto threshold = std::int32_t(value) % 3 + 2;
            leave(c, {{3, std::uint32_t(std::int32_t(count))}, {4, std::uint32_t(threshold - 2)}, {5, value}, {6, value - c.u32(seed)}});
            if (compare(c, count, threshold) >= 0) {
                const auto match = actor.match();
                leave(c, {{3, c.u32(match + 0x79f4)}, {4, 0x210c}, {5, match}, {6, UINT32_MAX},
                          {7, actor.side()}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}});
                c.call(0x10201180, 0x10287fac); c.set(18, std::uint32_t(-50)); suppress = true;
            }
        }
    }
    if (!suppress && compare(c, std::int8_t(c.r(18))) >= 0) {
        const auto match = actor.match();
        leave(c, {{3, c.u32(match + 0x79f4)}, {4, c.r(22)}, {5, match}, {6, c.u8(player + 0x2f)},
                  {7, actor.side()}, {8, c.u8(selected + 0x2f)}, {9, c.u8(selected + 0x27)}, {10, 0}});
        c.call(0x10201180, 0x10287fe4);
    }
    for (unsigned team = 0; compare(c, team, 2) < 0; ++team) {
        c.set(19, team); const auto first = actor.match() + 0x311c + team * 0x2350; c.set(20, 0); c.set(25, first);
        for (unsigned slot = 0; compare(c, slot, 20) < 0; ++slot) {
            const auto peer = first + slot * 0x1c4;
            if (compare(c, c.i8(peer + 0x19)) >= 0 && compare(c, c.i8(peer + 0x19), 10) <= 0 &&
                compare(c, c.i8(peer + 0x2c), 2) < 0 && compare_logical(c, peer, player) != 0) {
                enter(c, 0x1028804c, {peer, 1, 0}); accepts_response(c, peer, 1, 0);
                if (compare(c, std::int8_t(c.r(3))) != 0) {
                    enter(c, 0x10288064, {peer, c.u8(player + 0x105), c.u8(player + 0x106)});
                    set_coordinates(c, peer, c.r(4), c.r(5));
                }
            }
            c.set(20, slot + 1); c.set(25, peer + 0x1c4);
        }
        c.set(19, team + 1);
    }
    leave(c, {{3, actor.match()}, {4, player}, {5, selected}, {6, c.r(27)}, {7, UINT32_MAX},
              {8, frame.at(0x3a)}, {9, frame.at(0x39)}, {10, frame.at(0x38)}});
    c.call(0x101f5050, 0x102880ac);
    if (compare(c, std::int8_t(c.r(3))) != 0) {
        std::uint32_t found = UINT32_MAX; c.set(5, found); c.set(6, 0);
        for (std::uint32_t index = 0;; ++index) {
            const auto match = actor.match();
            leave(c, {{3, match}, {4, std::uint32_t(std::int32_t(std::int8_t(index)))}});
            if (compare(c, std::int8_t(index), c.i8(match + 0x1bc)) >= 0) break;
            const auto item = actor.match() + std::uint32_t(std::int8_t(index)) * 12;
            if (compare_logical(c, c.u32(item + 0xb4), player) == 0) {
                c.set(4, item); c.set(3, c.u8(item + 0xbc)); c.set(5, index); found = index;
                c.put8(item + 0xbc, c.r(3) + c.u8(frame.at(0x39)));
            }
            c.set(6, index + 1);
        }
        if (compare(c, std::int8_t(found), -1) == 0) {
            const auto match = actor.match(); const auto count = c.i8(match + 0x1bc); const auto item = match + std::uint32_t(count) * 12;
            c.put32(item + 0xb4, player); c.put32(item + 0xb8, selected);
            c.put8(item + 0xbe, c.u8(frame.at(0x3a))); c.put8(item + 0xbd, c.r(27));
            const auto amount = c.u8(frame.at(0x39)); const auto raw_count = c.u8(match + 0x1bc);
            leave(c, {{3, item}, {4, raw_count}, {5, match}, {6, match}, {7, amount}});
            c.put8(match + 0x1bc, raw_count + 1); c.put8(item + 0xbc, amount);
        }
    }
    auto depth_now = std::int8_t(c.r(18));
    bool recurse;
    if (compare(c, depth_now, 10) < 0 && compare(c, depth_now) >= 0) recurse = true;
    else recurse = compare(c, depth_now, -40) < 0;
    if (recurse) {
        c.set(3, actor.match());
        if (compare(c, c.i8(c.r(3) + 0x1bc), 5) < 0) {
            enter(c, 0x10288208, {selected, std::uint32_t(std::int32_t(std::int8_t(depth_now + 1)))});
            resolve_peer_contact(c, selected, c.r(4));
        }
    }
    return result(c, 1);
}
}
