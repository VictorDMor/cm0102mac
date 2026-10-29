#include "match_pl_ball_action_detail.hpp"
#include <array>

namespace cm::aot::match_pl::detail {
using namespace guest;

void BallAction::contextual_commentary() {
    for (int action : {18, 17, 16, 15}) if (compare(c, c.i16(player() + 0x19c), action) == 0) return;
    for (int action : {8, 6, 7, 94}) if (compare(c, kind(), action) == 0) return;
    if (compare(c, byte(0xed)) != 0) return;
    advance(0x1024c6dc); c.set(5, sample()); c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(27)))));
    c.set(4, shifted_sign(high(c.r(5), 0x66666667), 3)); const auto threshold = 30 - std::int32_t(c.r(5)) % 20;
    c.s.wr<1>(reg::xer_ca, std::uint32_t(30 - threshold) <= 30);
    if (compare(c, std::int8_t(c.r(27)), threshold) < 0 || compare(c, c.i8(match() + 0x791f), 3) >= 0) return;
    enter(c, 0x1024c740, {player()}); current_role(c, player()); if (compare(c, std::int16_t(c.r(3)), 1) == 0) return;
    advance(0x1024c76c); if (compare(c, modulo(3)) != 0) return;
    const auto pitch_metric = [&](bool second) {
        const bool first_side = compare(c, std::int8_t(actor.side())) == 0;
        c.set(3, std::uint32_t(int(actor.column()) - 4));
        c.call(0x10775540, second ? (first_side ? 0x1024c89c : 0x1024c8e0) : (first_side ? 0x1024c7c0 : 0x1024c804));
        c.set(13, std::uint32_t(std::int32_t(std::int8_t(c.r(3)))));
        c.set(3, std::uint32_t(int(actor.row()) - (first_side ? 11 : 0)));
        c.call(0x10775540, second ? (first_side ? 0x1024c8b0 : 0x1024c8f4) : (first_side ? 0x1024c7d4 : 0x1024c818));
        const auto row = int(std::int8_t(c.r(3))), column = int(std::int8_t(c.r(13))); c.set(3, c.toc(-0x6424));
        c.set_f(2, c.f32(c.r(3) + std::uint32_t(row * 36 + column * 4)));
    };
    const auto time = [&] {
        c.set_f(0, c.f32(singles + 0x8c)); c.set_f(1, c.f64(doubles + 0xc0));
        c.set_f(0, fp.fmuls(c.f(0), c.f(2))); c.set_f(0, fp.fadd(c.f(1), c.f(0))); c.put64(at(0x94), fp.fctiwz(0, c.f(0)));
        const auto value = std::int8_t(c.u32(at(0x98))); shifted_sign(high(std::uint32_t(std::int32_t(value)), 0x66666667), 1);
        return int(value) / 5 * 5;
    };
    pitch_metric(false); auto duration = time();
    if (compare(c, duration, 1) < 0) duration = 1; else { pitch_metric(true); duration = time(); }
    c.set(13, std::uint32_t(std::int32_t(std::int8_t(duration))));
    const auto announce = [&](unsigned id, std::uint32_t back) {
        leave(c, {{3, c.u32(match() + 0x79f4)}, {4, id}, {5, match()}, {6, c.u8(player() + 0x2f)},
                  {7, actor.side()}, {8, UINT32_MAX}, {9, UINT32_MAX}, {10, std::uint32_t(std::int32_t(std::int8_t(c.r(13))))}});
        c.call(0x10201180, back);
    };
    const auto left = [&] {
        return (compare(c, std::int8_t(actor.side())) == 0 && compare(c, actor.column(), 2) < 0)
            || (compare(c, actor.side(), 1) == 0 && compare(c, actor.column(), 6) > 0);
    };
    const auto right = [&] {
        return (compare(c, std::int8_t(actor.side())) == 0 && compare(c, actor.column(), 6) > 0)
            || (compare(c, actor.side(), 1) == 0 && compare(c, actor.column(), 2) < 0);
    };
    bool advanced = compare(c, std::int8_t(actor.side())) == 0 && compare(c, actor.row(), 7) > 0;
    if (!advanced) advanced = compare(c, actor.side(), 1) == 0 && compare(c, actor.row(), 4) < 0;
    if (advanced) {
        if (left()) announce(0x2160, 0x1024c9e8);
        else if (right()) announce(0x2162, 0x1024ca4c);
        else if (!opposite_area(player())) announce(0x2161, 0x1024cb34);
        return;
    }
    bool deep = compare(c, actor.side(), 1) == 0 && compare(c, actor.row(), 7) > 0;
    if (!deep) deep = compare(c, std::int8_t(actor.side())) == 0 && compare(c, actor.row(), 4) < 0;
    if (deep) {
        if (left()) announce(0x215a, 0x1024cbd0);
        else if (right()) announce(0x215c, 0x1024cc34);
        else announce(0x215b, 0x1024cc60);
        return;
    }
    if (left()) { announce(0x215d, 0x1024ccc4); return; }
    if (right()) { announce(0x215f, 0x1024cd28); return; }
    c.set(3, std::uint32_t(int(actor.column()) - 4)); c.call(0x10775540, 0x1024cd3c);
    if (compare(c, std::int32_t(c.r(3)), 2) >= 0) return;
    if (compare(c, c.u8(player() + 0x106), 5) != 0 && compare(c, c.u8(player() + 0x106), 6) != 0) return;
    advance(0x1024cd7c); if (compare(c, binary()) != 0) announce(0x215e, 0x1024cdd0);
}

void BallAction::direction_flags() {
    const bool crossing = compare(c, kind(), 8) == 0 || compare(c, kind(), 6) == 0 || compare(c, kind(), 7) == 0 || compare(c, kind(), 94) == 0;
    const auto abs = [&](int value, std::uint32_t back) { c.set(3, std::uint32_t(value)); c.call(0x10775540, back); return std::int32_t(c.r(3)); };
    const auto left = [&] {
        return (compare(c, std::int8_t(actor.side())) == 0 && compare(c, to_column(), from_column()) < 0)
            || (compare(c, actor.side(), 1) == 0 && compare(c, to_column(), from_column()) > 0);
    };
    const auto right = [&] {
        return (compare(c, std::int8_t(actor.side())) == 0 && compare(c, to_column(), from_column()) > 0)
            || (compare(c, actor.side(), 1) == 0 && compare(c, to_column(), from_column()) < 0);
    };
    if (crossing) {
        c.set(28, 0);
        bool near_left = compare(c, std::int8_t(actor.side())) == 0 && compare(c, from_column(), 3) < 0;
        if (!near_left) near_left = compare(c, actor.side(), 1) == 0 && compare(c, from_column(), 5) > 0;
        if (near_left) c.set(28, 1);
        else {
            bool near_right = compare(c, actor.side(), 1) == 0 && compare(c, from_column(), 3) < 0;
            if (!near_right) near_right = compare(c, std::int8_t(actor.side())) == 0 && compare(c, from_column(), 5) > 0;
            if (near_right) c.set(28, 2);
        }
        c.set(13, std::uint32_t(abs(int(from_column()) - 4, 0x1024d3c0)));
        const auto displacement = abs(int(from_column()) - to_column(), 0x1024d3d8);
        const auto delta = std::int8_t(c.r(13) - std::uint32_t(displacement));
        if (compare(c, delta) > 0) c.set(28, c.r(28) | 8);
        else if (compare(c, delta) < 0) c.set(28, c.r(28) | 4);
        return;
    }
    c.set(29, 0);
    if (compare_logical(c, receiver()) != 0) {
        enter(c, 0x1024ce14, {receiver()}); current_role(c, receiver()); if (compare(c, std::int16_t(c.r(3)), 1) == 0) c.set(29, std::uint32_t(-128));
    }
    bool forward = compare(c, std::int8_t(actor.side())) == 0 && compare(c, to_row(), from_row()) > 0;
    if (!forward) forward = compare(c, actor.side(), 1) == 0 && compare(c, to_row(), from_row()) < 0;
    if (forward) c.set(29, std::uint32_t(std::int32_t(std::int8_t(c.r(29) | 1))));
    else {
        bool backward = compare(c, std::int8_t(actor.side())) == 0 && compare(c, to_row(), from_row()) < 0;
        if (!backward) backward = compare(c, actor.side(), 1) == 0 && compare(c, to_row(), from_row()) > 0;
        if (backward) c.set(29, std::uint32_t(std::int32_t(std::int8_t(c.r(29) | 8))));
    }
    if (compare(c, abs(int(to_row()) - from_row(), 0x1024cee4), 2) < 0) {
        const auto lateral = [&](bool leftward) {
            c.set(13, std::uint32_t(abs(int(to_column()) - from_column(), leftward ? 0x1024cf40 : 0x1024cfc4)));
            return compare(c, abs(int(to_row()) - from_row(), leftward ? 0x1024cf5c : 0x1024cfe0), std::int32_t(c.r(13))) <= 0;
        };
        if (left() && lateral(true)) c.set(29, std::uint32_t(std::int32_t(std::int8_t(c.r(29) | 2))));
        else if (right() && lateral(false)) c.set(29, std::uint32_t(std::int32_t(std::int8_t(c.r(29) | 4))));
    }
    if (!opposite_area(player())) {
        leave(c, {{3, match()}, {4, unsigned(std::int8_t(actor.side()) == 0)}, {5, c.r(26)}, {6, c.u8(at(0x187))}});
        c.call(0x101eea30, 0x1024d0d0); if (compare(c, std::int8_t(c.r(3))) != 0) c.set(29, std::uint32_t(std::int32_t(std::int8_t(c.r(29) | 0x10))));
    }
    enter(c, 0x1024d0e8, {player()}); in_attacking_third(c, player());
    bool progressed = compare(c, std::int8_t(c.r(3))) != 0;
    if (!progressed) progressed = compare(c, std::int8_t(actor.side())) == 0 && compare(c, to_row(), int(from_row()) + 3) > 0;
    if (!progressed) progressed = compare(c, actor.side(), 1) == 0 && compare(c, to_row(), int(from_row()) - 3) < 0;
    if (progressed) {
        const auto wide = [&](std::uint32_t back) {
            leave(c, {{3, match()}, {4, c.r(26)}}); c.call(0x101ee9c0, back); return compare(c, std::int8_t(c.r(3))) != 0;
        };
        const auto near_left = [&] {
            return (compare(c, std::int8_t(actor.side())) == 0 && compare(c, to_column(), actor.column()) < 0 && compare(c, to_column(), 2) < 0)
                || (compare(c, actor.side(), 1) == 0 && compare(c, to_column(), actor.column()) > 0 && compare(c, to_column(), 6) > 0);
        };
        const auto near_right = [&] {
            return (compare(c, std::int8_t(actor.side())) == 0 && compare(c, to_column(), actor.column()) > 0 && compare(c, to_column(), 6) > 0)
                || (compare(c, actor.side(), 1) == 0 && compare(c, to_column(), actor.column()) < 0 && compare(c, to_column(), 2) < 0);
        };
        if (wide(0x1024d14c) && near_left()) c.set(29, std::uint32_t(std::int32_t(std::int8_t(c.r(29) | 0x20))));
        else if (wide(0x1024d1c4) && near_right()) c.set(29, std::uint32_t(std::int32_t(std::int8_t(c.r(29) | 0x40))));
    }
    bool goal = compare(c, std::int8_t(actor.side())) == 0 && compare(c, to_column(), 4) == 0 && compare(c, c.u8(at(0x187)), 12) == 0;
    if (!goal) goal = compare(c, actor.side(), 1) == 0 && compare(c, to_column(), 4) == 0 && compare(c, to_row(), -1) == 0;
    if (goal) {
        c.set_f(0, c.f64(doubles + 0xc8)); fp.fcmp(0, metric(), c.f(0));
        if (c.cr(0) & 8) { c.set(24, compare(c, kind(), 61) == 0 ? 92 : 93); return; }
    }
    if (compare(c, c.i8(match() + 0x791f), 4) < 0 && compare(c, kind(), 93) != 0 && compare(c, kind(), 92) != 0
        && compare(c, to_column(), 4) == 0 && (compare(c, to_row(), -1) == 0 || compare(c, c.u8(at(0x187)), 12) == 0))
        c.put16(at(0xee), 0x211f);
}

void BallAction::action_commentary() {
    struct Publication { unsigned id; std::uint32_t back; };
    const auto flag = [&](unsigned mask) { return compare(c, int(c.r(29) & mask)) != 0; };
    const auto cross = [&](unsigned mask) { return compare(c, int(c.r(28) & mask)) != 0; };
    const auto keeper = [&] { return flag(0x80) && !flag(1); };
    const auto matrix = [&](unsigned base, unsigned fallback, std::uint32_t first) {
        constexpr std::array masks{0x10u, 0x20u, 0x40u, 2u, 4u, 1u, 8u};
        constexpr std::array increments{4u, 5u, 6u, 1u, 2u, 0u, 3u};
        for (unsigned i = 0; i < masks.size(); ++i) {
            if (flag(masks[i])) { receiver_message(base + increments[i], first + i * 0x34, true); return; }
        }
        receiver_message(fallback, first + 0x164, true);
    };
    const auto variant = [&](unsigned base, unsigned fallback, const std::array<std::uint32_t, 4>& backs) {
        // The control byte and previous possession state select four text
        // families. Selection itself draws no random numbers.
        const bool controlled = compare(c, c.i8(c.u32(at(0x178)))) != 0;
        const bool previous = compare(c, std::int8_t(c.r(14))) != 0;
        const unsigned index = (controlled ? 2u : 0u) + (previous ? 1u : 0u);
        matrix(base + index * 7, fallback, backs[index]);
    };
    const auto crossing = [&](bool low_cross) {
        constexpr std::array<Publication, 13> high{{
            {0x20c3, 0x1024d910}, {0x20c1, 0x1024d944}, {0x20bf, 0x1024d970},
            {0x20bd, 0x1024d9a4}, {0x20bb, 0x1024d9d8}, {0x20b9, 0x1024da04},
            {0x20c4, 0x1024da48}, {0x20c2, 0x1024da7c}, {0x20c0, 0x1024daa8},
            {0x20be, 0x1024dadc}, {0x20bc, 0x1024db10}, {0x20ba, 0x1024db3c}, {0x1f73, 0x1024db68}}};
        constexpr std::array<Publication, 13> low{{
            {0x20cf, 0x1024dbac}, {0x20cd, 0x1024dbe0}, {0x20cb, 0x1024dc0c},
            {0x20c9, 0x1024dc40}, {0x20c7, 0x1024dc74}, {0x20c5, 0x1024dca0},
            {0x20d0, 0x1024dce4}, {0x20ce, 0x1024dd18}, {0x20cc, 0x1024dd44},
            {0x20ca, 0x1024dd78}, {0x20c8, 0x1024ddac}, {0x20c6, 0x1024ddd8}, {0x21e4, 0x1024de04}}};
        unsigned choice = 12;
        if (cross(1)) choice = 0; else if (cross(2)) choice = 6;
        if (choice != 12) {
            if (compare(c, std::int8_t(c.r(14))) == 0) choice += 3;
            if (cross(8)) {} else if (cross(4)) ++choice; else choice += 2;
        }
        const auto publication = low_cross ? low[choice] : high[choice]; player_message(publication.id, publication.back);
    };
    const auto opposing_target = [&](std::uint32_t back) {
        leave(c, {{3, match()}, {4, unsigned(std::int8_t(actor.side()) == 0)},
                  {5, c.r(26)}, {6, c.u8(at(0x187))}}); c.call(0x101eea30, back);
        return compare(c, std::int8_t(c.r(3))) != 0;
    };
    const auto outside_row = [&] { return compare(c, to_row(), 12) >= 0 || compare(c, to_row()) < 0; };

    const auto index = std::uint32_t(int(kind()) - 6); compare_logical(c, index, 88);
    switch (kind()) {
    case 6: case 8: case 94: crossing(false); return;
    case 7: crossing(true); return;
    case 9: {
        bool behind_goal = compare(c, to_row(), 12) >= 0 && compare(c, actor.side(), 1) == 0;
        if (!behind_goal) behind_goal = compare(c, to_row()) < 0 && compare(c, std::int8_t(actor.side())) == 0;
        if (behind_goal) {
            bool central = compare(c, to_column(), 4) == 0;
            if (!central) { advance(0x1024d484); central = compare(c, binary(4)) == 0; }
            player_message(central ? 0x211a : 0x210e, central ? 0x1024d4dc : 0x1024d508, true);
        } else {
            const bool area = defensive_area(player()); player_message(area ? 0x1f70 : 0x2157, area ? 0x1024d60c : 0x1024d5e0, true);
        }
        return;
    }
    case 11:
        if (keeper()) { receiver_message(0x214a, 0x1024ef4c); return; }
        if (opposite_area(receiver()) && opposite_area(player())) {
            c.set_f(0, c.f64(doubles + 0xc8)); fp.fcmp(0, metric(), c.f(0));
            if (c.cr(0) & 8) { receiver_message(0x2021, 0x1024f0fc, true); return; }
        }
        variant(0x205a, 0x1f69, {0x1024f148, 0x1024f2f8, 0x1024f4a8, 0x1024f640}); return;
    case 30:
        if (keeper()) { receiver_message(0x214c, 0x1024f7e0); return; }
        variant(0x2092, 0x1f6a, {0x1024f82c, 0x1024f9dc, 0x1024fb8c, 0x1024fd24}); return;
    case 31:
        if (keeper()) { receiver_message(0x214a, 0x1024e1b0); return; }
        if (compare(c, c.i8(match() + 0x791f)) == 0)
            variant(0x2022, 0x1f67, {0x1024e20c, 0x1024e3bc, 0x1024e56c, 0x1024e704});
        else variant(0x203e, 0x1f67, {0x1024e8b4, 0x1024ea64, 0x1024ec14, 0x1024edac});
        return;
    case 52:
        c.set_f(0, c.f64(doubles + 8)); fp.fcmp(0, metric(), c.f(0));
        if (c.cr(0) & 8) player_message(0x2130, 0x1024dff8, true);
        else if (opposing_target(0x1024e020)) player_message(0x1f80, 0x1024e07c, true);
        else player_message(0x2131, 0x1024e050, true);
        return;
    case 54: {
        const auto present = compare_logical(c, receiver()) != 0;
        if (present) receiver_message(0x1f7b, 0x1024e148, true); else player_message(0x1f7c, 0x1024e174, true); return;
    }
    case 55:
        enter(c, 0x1024e0b8, {player(), c.r(26)}); wide_column(c, to_column());
        if (compare(c, std::int8_t(c.r(3))) != 0) player_message(0x21bc, 0x1024e0e8, true);
        else player_message(0x1f7d, 0x1024e114, true);
        return;
    case 60:
        if (keeper()) { receiver_message(0x214c, 0x1024fec4); return; }
        variant(0x2076, 0x1f6b, {0x1024ff10, 0x102500c0, 0x10250270, 0x10250408}); return;
    case 61:
        if (keeper()) { receiver_message(0x2148, 0x1024de6c); return; }
        if (opposing_target(0x1024de94)) { receiver_message(0x21d5, 0x1024dec4); return; }
        if (flag(2)) receiver_message(0x20b6, 0x1024def8);
        else if (flag(4)) receiver_message(0x20b7, 0x1024df2c);
        else if (flag(1)) receiver_message(0x20b5, 0x1024df60);
        else if (flag(8)) receiver_message(0x20b8, 0x1024df94);
        else receiver_message(0x1f6d, 0x1024dfc0);
        return;
    case 62: player_message(0x2134, 0x1024d638, true); return;
    case 90: player_message(0x1f7e, 0x1024e0a8, true); return;
    case 91:
        if (outside_row()) {
            bool central = compare(c, to_column(), 4) == 0;
            if (!central) { advance(0x1024d684); central = compare(c, binary(4)) == 0; }
            player_message(central ? 0x2119 : 0x210d, central ? 0x1024d6dc : 0x1024d708, true);
        } else player_message(0x1f6f, 0x1024d734, true);
        return;
    case 92: player_message(0x1f71, 0x1024de30, true); return;
    case 93: {
        constexpr std::array masks{0x10u, 0x20u, 0x40u, 2u, 4u, 1u, 8u};
        constexpr std::array ids{0x20b2u, 0x20b3u, 0x20b4u, 0x20afu, 0x20b0u, 0x20aeu, 0x20b1u};
        for (unsigned i = 0; i < masks.size(); ++i)
            if (flag(masks[i])) { player_message(ids[i], 0x1024d768 + i * 0x34, true); return; }
        player_message(0x1f72, 0x1024d8cc, true); return;
    }
    default:
        leave(c, {{3, c.toc(-0x7bb8)}, {4, c.toc(-0x51e4) + 0xbf}, {5, std::uint32_t(std::int32_t(kind()))}});
        c.call(0x1077b720, 0x10250584);
        leave(c, {{3, c.toc(-0x51e4) + 0x8f}, {4, 0}, {5, 0}, {6, at(0x58)}, {7, at(0x50)}});
        c.call(0x10739890, 0x102505a0); c.call(0x10768810, 0x102505a4);
        leave(c, {{5, c.r(3)}, {3, c.toc(-0x7ba8)}, {4, c.toc(-0x51e4) + 0x9c}, {6, at(0x58)}, {7, at(0x50)}, {8, 0x2a31}});
        c.call(0x1077b720, 0x102505c4);
        leave(c, {{3, c.toc(-0x51e4) + 0xa9}, {4, c.toc(-0x7ba8)}, {5, 0}}); c.call(0x10769b60, 0x102505d8);
        c.set(4, c.toc(-0x7bac)); c.set(0, 0); c.put8(c.r(4), 0); c.set(3, UINT32_MAX); c.call(0x1076f7d0, 0x102505ec);
    }
}
}
