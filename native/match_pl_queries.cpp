#include "match_pl_detail.hpp"

namespace cm::aot::match_pl {
using namespace guest;

std::uint32_t at_attacking_goal(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x10237420, 0x40, {});
    const Player actor{c, player};
    const auto constants = c.u32(c.r(2) - 0x51e0);
    c.set(4, constants);
    const bool first = compare(c, std::int8_t(actor.side())) == 0;
    const auto goal_row = first ? 11 : 0;
    if (compare(c, first ? c.u8(player + 0x106) : actor.row(), goal_row) == 0 &&
        compare(c, actor.column(), 3) >= 0 && compare(c, actor.column(), 5) <= 0)
        return result(c, 1);
    c.set_f(1, c.f32(constants));
    c.set_f(2, c.f32(constants));
    leave(c, {{4, 4}, {5, std::uint32_t(goal_row)}, {6, 0}, {9, 0}});
    enter(c, first ? 0x1023748c : 0x102374e0, {player, 4, std::uint32_t(goal_row), 0});
    accepts_target(c, player, 4, std::uint32_t(goal_row), 0, 0, c.f(1), c.f(2));
    return c.r(3);
}

std::int32_t team_instruction(Cpu& c, std::uint32_t player) {
    const Player actor{c, player};
    const bool second = compare(c, std::int8_t(actor.side())) != 0;
    const auto value = c.i8(actor.match() + (second ? 0xe03d : 0xe073));
    return std::int32_t(result(c, std::uint32_t(std::int32_t(value))));
}

namespace {
bool goal_area(Cpu& c, std::int8_t side, std::int8_t column, std::int8_t row) {
    const bool second = compare(c, side, 1) == 0;
    if (compare(c, column, 2) < 0 || compare(c, column, 6) > 0) return false;
    if (compare(c, row, second ? 10 : 0) < 0) return false;
    return compare(c, row, second ? 11 : 1) <= 0;
}
}

std::uint32_t in_goal_area(Cpu& c, std::uint32_t player, std::int8_t side) {
    const Player actor{c, player};
    const bool second = compare(c, side, 1) == 0;
    c.set(4, 0);
    // Keep the reference's short-circuit compares; CR0 reaches later callers.
    bool inside = false;
    if (compare(c, actor.column(), 2) >= 0 && compare(c, actor.column(), 6) <= 0 &&
        compare(c, actor.row(), second ? 10 : 0) >= 0 && compare(c, actor.row(), second ? 11 : 1) <= 0)
        inside = true;
    c.set(4, inside);
    return result(c, inside);
}

std::uint32_t coordinates_in_goal_area(Cpu& c, std::int8_t side, std::int8_t column, std::int8_t row) {
    return result(c, goal_area(c, side, column, row));
}

std::uint32_t on_goal_line(Cpu& c, std::uint32_t player) {
    const Player actor{c, player};
    const bool goal = compare(c, actor.row()) == 0 || compare(c, c.u8(player + 0x106), 11) == 0;
    return result(c, goal);
}

std::uint32_t in_defensive_third(Cpu& c, std::uint32_t player) {
    const Player actor{c, player};
    if (compare(c, std::int8_t(actor.side())) == 0 && compare(c, actor.row(), 3) < 0) return result(c, 1);
    if (compare(c, actor.side(), 1) == 0 && compare(c, actor.row(), 8) > 0) return result(c, 1);
    return result(c, 0);
}

std::uint32_t wide_column(Cpu& c, std::int8_t column) {
    const bool outside = compare(c, column, 2) < 0 || compare(c, column, 6) > 0;
    return result(c, outside);
}
std::uint32_t in_wide_column(Cpu& c, std::uint32_t player) {
    return wide_column(c, Player{c, player}.column());
}

std::uint32_t in_attacking_third(Cpu& c, std::uint32_t player) {
    const Player actor{c, player};
    if (compare(c, actor.side(), 1) == 0 && compare(c, actor.row(), 3) < 0) return result(c, 1);
    if (compare(c, std::int8_t(actor.side())) == 0 && compare(c, actor.row(), 8) > 0) return result(c, 1);
    return result(c, 0);
}
std::uint32_t in_attacking_band(Cpu& c, std::uint32_t player) {
    const Player actor{c, player};
    if (compare(c, actor.side(), 1) == 0 && compare(c, actor.row(), 4) < 0) return result(c, 1);
    if (compare(c, std::int8_t(actor.side())) == 0 && compare(c, actor.row(), 7) > 0) return result(c, 1);
    return result(c, 0);
}
std::uint32_t in_own_half(Cpu& c, std::uint32_t player) {
    const Player actor{c, player};
    if (compare(c, actor.side(), 1) == 0 && compare(c, actor.row(), 6) >= 0) return result(c, 1);
    if (compare(c, std::int8_t(actor.side())) == 0 && compare(c, actor.row(), 6) < 0) return result(c, 1);
    return result(c, 0);
}
std::uint32_t row_in_own_half(Cpu& c, std::uint32_t player, std::int8_t row) {
    const Player actor{c, player};
    if (compare(c, actor.side(), 1) == 0 && compare(c, row, 6) >= 0) return result(c, 1);
    if (compare(c, std::int8_t(actor.side())) == 0 && compare(c, row, 6) < 0) return result(c, 1);
    return result(c, 0);
}

std::uint32_t team_crossed_half(Cpu& c, std::uint32_t player) {
    const Player actor{c, player};
    const bool second = compare(c, actor.side(), 1) == 0;
    const auto row = c.i8(actor.match() + 0x791e);
    if (second) {
        leave(c, {{4, std::uint32_t(std::int32_t(row) >> 31)}, {5, std::uint32_t(std::int32_t(row))}});
        c.s.wr<1>(reg::xer_ca, std::uint32_t(std::int32_t(row)) >= 6);
    } else c.set(4, 6);
    return result(c, second ? row >= 6 : row < 6);
}

std::uint32_t both_roles_masked(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x1028abc0, 0x40, {.first_gpr = 31});
    Kept<31> retained(c, player);
    const auto role = [&](std::uint32_t target, std::uint32_t back) {
        const auto match = Player{c, retained}.match();
        if (compare_logical(c, match) == 0) { c.set(3, 0); return 0u; }
        leave(c, {{3, match}, {4, retained}});
        c.call(target, back);
        return c.r(3);
    };
    if (compare(c, std::uint32_t(std::int32_t(std::int16_t(role(0x101e3d50, 0x1028abec)))) & 0x880) == 0)
        return result(c, 0);
    const auto mask = std::uint32_t(std::int32_t(std::int16_t(role(0x101e3de0, 0x1028ac18)))) & 0x880;
    return result(c, compare(c, mask) != 0);
}

std::int32_t role_rank(Cpu& c, std::uint32_t flags) {
    constexpr std::array bits{1u, 4u, 8u, 16u, 32u, 64u};
    for (unsigned rank = 0; rank < bits.size(); ++rank)
        if (compare(c, flags & bits[rank]) != 0) return std::int32_t(result(c, rank));
    return std::int32_t(result(c, 3));
}

std::int32_t role_distance(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x1028ac50, 0x40, {.first_gpr = 30});
    Kept<30> retained(c, player);
    const auto role = [&](std::uint32_t target, std::uint32_t back) {
        const auto match = Player{c, retained}.match();
        if (compare_logical(c, match) != 0) {
            leave(c, {{3, match}, {4, retained}}); c.call(target, back);
        } else c.set(3, 0);
        return c.r(3);
    };
    const auto current = role(0x101e3d50, 0x1028ac80);
    enter(c, 0x1028ac94, {retained, current});
    Kept<31> first(c, std::uint32_t(role_rank(c, current)));
    const auto previous = role(0x101e3de0, 0x1028acb0);
    enter(c, 0x1028acc4, {retained, previous});
    const auto second = role_rank(c, previous);
    const auto distance = std::int32_t(first) - second;
    c.set(3, std::uint32_t(distance)); c.call(0x10775540, 0x1028acd4);
    return std::int32_t(result(c, std::uint32_t(std::int32_t(std::int8_t(c.r(3))))));
}
}
