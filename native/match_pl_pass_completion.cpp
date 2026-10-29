#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"
#include "match_pl_pass_sampling.hpp"

namespace cm::aot::match_pl {
using namespace guest;

// Finish an action against a receiver, retaining the action core's in/out flag.
void complete_targeted_action(Cpu& c, std::uint32_t player, std::uint32_t kind,
                             std::uint32_t receiver, std::uint32_t control,
                             std::uint32_t column, std::uint32_t row) {
    Frame frame(c, 0x10245220, 0xc0, {.first_gpr = 20, .gpr_offset = -0x40, .first_fpr = 30});
    const auto distances = c.toc(-0x6424), doubles = c.toc(-0x51e8), singles = c.toc(-0x51e0),
               seed = c.toc(-0x7b90), begin = c.toc(-0x7b8c), end = c.toc(-0x7b88), cursor = c.toc(-0x7b84);
    leave(c, {{9, column}, {8, receiver}, {10, row}});
    keep(c, {{21, row}, {29, receiver}, {30, player}, {20, kind},
             {25, distances}, {24, doubles}, {27, seed},
             {23, begin}, {31, 0}, {22, end}, {28, cursor}, {26, singles}});
    const Player actor{c, player};
    const auto high_product = [](std::uint32_t value, std::int32_t multiplier) {
        return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * multiplier) >> 32);
    };
    const auto shifted_sign = [&](std::int32_t high, unsigned bits) {
        c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << bits) - 1)));
        return std::uint32_t(high < 0);
    };
    const auto advance = [&](std::uint32_t back) {
        const auto next = c.u32(cursor) + 2; c.set(3, next);
        keep(c, {{20, c.u8(receiver + 0x10a)}, {21, c.u8(receiver + 0x108)}});
        const auto order = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
        if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
    };
    c.put8(frame.at(0xe7), control); c.set(6, kind); c.set(7, frame.at(0xe7));
    c.set(3, c.u8(player + 0x2c)); c.put8(player + 0x2c, c.r(3) + 1);
    leave(c, {{3, player}, {4, c.u8(player + 0x105)}, {5, c.u8(player + 0x106)}});
    enter(c, 0x1024529c, {c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)});
    resolve_ball_action(c, c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10));
    if (compare(c, std::int8_t(c.r(3))) == 0) return;
    if (compare(c, std::int8_t(c.r(20)), 54) != 0) {
        leave(c, {{3, actor.match()}, {4, actor.side()}, {5, c.u8(player + 0x106)}, {6, c.r(21)}, {7, 0}});
        c.call(0x101f5230, 0x102452c8); c.set(31, c.r(3));
    }
    if (compare_logical(c, c.r(31)) != 0) {
        const auto goal_distance = [&](unsigned destination, bool second_read) {
            const Player defender{c, c.r(31)};
            const bool first = compare(c, std::int8_t(defender.side())) == 0;
            c.set(3, std::uint32_t(std::int32_t(defender.column()) - 4));
            c.call(0x10775540, second_read ? first ? 0x10245378 : 0x102453b8 : first ? 0x102452f0 : 0x10245330);
            c.set(20, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
            c.set(3, std::uint32_t(std::int32_t(defender.row()) - (first ? 11 : 0)));
            c.call(0x10775540, second_read ? first ? 0x1024538c : 0x102453cc : first ? 0x10245304 : 0x10245344);
            c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))) * 36);
            const auto offset = c.r(3) + std::uint32_t(std::int32_t(std::int8_t(c.r(20)))) * 4;
            c.set_f(destination, c.f32(distances + offset));
        };
        goal_distance(31, false); goal_distance(30, true);
        sample_interception_distance(c, frame, 24, 0x40, singles, doubles,
                                     seed, begin, end, cursor, 0x10245424, 0x1024555c);
        if (compare(c, std::int32_t(c.r(24))) == 0) {
            c.put32(frame.at(0x38), UINT32_MAX); c.put32(frame.at(0x3c), UINT32_MAX);
            leave(c, {{3, actor.match()}, {4, UINT32_MAX}, {5, c.r(31)}, {6, 0}, {7, 0}, {8, 8},
                      {9, c.u8(c.r(31) + 0x105)}, {10, c.u8(c.r(31) + 0x106)}}); c.call(0x101d68f0, 0x1024565c);
        } else {
            compare_logical(c, c.r(31));
            leave(c, {{3, c.u32(actor.match() + 0x79f4)}, {4, 0x1f78}, {5, actor.match()}, {6, c.u8(c.r(31) + 0x2f)},
                      {7, actor.side()}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}}); c.call(0x10201180, 0x10245690);
            c.set(4, actor.side()); c.set(3, 7); c.set(5, actor.match());
            c.set(4, std::uint32_t(std::int32_t(std::int8_t(c.r(4))) * 54)); c.set(5, c.r(5) + 0x10000 + c.r(4));
            c.set(4, c.u8(c.r(5) - 0x1fb6)); c.put8(c.r(5) - 0x1fb6, c.r(4) + 1);
            c.set(4, c.u8(c.r(31) + 0x13)); c.put8(c.r(31) + 0x13, c.r(4) + 1);
            c.set(4, c.u8(player + 1)); c.put8(player + 1, c.r(4) - 1);
            c.set(5, c.u8(c.r(31) + 0x105)); c.set(4, actor.match()); c.put8(c.r(4) + 0x791d, c.r(5));
            c.set(5, c.u8(c.r(31) + 0x106)); c.set(4, actor.match()); c.put8(c.r(4) + 0x791e, c.r(5));
            c.set(4, actor.match()); c.put8(c.r(4) + 0x7928, c.r(3)); c.set(3, unsigned(std::int8_t(actor.side()) == 0));
            c.set(4, actor.match()); c.put8(c.r(4) + 0x7924, c.r(3)); c.set(3, actor.match() + 0x10000); c.put32(c.r(3) - 0x2000, 0); return;
        }
    }
    c.set(3, actor.match()); if (compare(c, c.i8(c.r(3) + 0x7928)) != 0) return;
    if (compare(c, c.i8(frame.at(0xe7))) == 0) return;
    c.set(3, actor.match() + 0x10000); c.put32(c.r(3) - 0x2008, receiver);
    c.set(6, actor.side()); c.set(5, actor.match()); compare(c, std::int8_t(c.r(6)), -1); c.put8(c.r(5) + 0x7921, c.r(6));
    if (std::int8_t(c.r(6)) != -1) {
        const auto side = std::int32_t(std::int8_t(c.r(6))); const auto opposite = unsigned(side == 0);
        c.set(3, std::uint32_t(side)); c.set(4, c.r(5) + c.r(3)); c.set(3, c.u8(c.r(4) + 0x77bc)); c.put8(c.r(4) + 0x77bc, c.r(3) + 1);
        c.set(3, c.r(5) + opposite); const auto value = c.i8(c.r(3) + 0x77bc);
        if (compare(c, value) > 0) {
            c.set(4, c.r(5) + opposite); c.set(3, c.u8(c.r(4) + 0x77bc)); c.put8(c.r(4) + 0x77bc, c.r(3) - 10);
        }
    }
    if (compare_logical(c, receiver) == 0) return;
    if (compare(c, c.i8(receiver + 0x2c)) != 0) return;
    advance(0x102457f4); c.set(3, c.u32(cursor)); c.set(4, 0x66666667); c.set(5, c.u32(seed)); c.set(6, c.u16(c.r(3)));
    c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(20))))); c.set(6, c.r(6) + c.r(5));
    const auto high = high_product(c.r(6), 0x66666667); c.set(4, std::uint32_t(high >> 5)); c.set(5, shifted_sign(high, 5));
    c.set(4, std::uint32_t(std::int32_t(c.r(6)) % 80));
    if (compare(c, std::int8_t(c.r(20)) * std::int8_t(c.r(21)), std::int32_t(c.r(4))) > 0) {
        enter(c, 0x10245848, {receiver}); dispatch_action(c, receiver);
    }
}
}
