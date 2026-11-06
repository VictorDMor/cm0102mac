#include "match_pl_shot_event_detail.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

bool in_goal_area_for_side(Cpu& c, std::uint32_t player, bool attacking_away_goal) {
    const auto compare_coordinate = [&](unsigned offset, int limit) {
        const auto coordinate = c.i8(player + offset);
        c.set(0, std::uint32_t(coordinate));
        c.compare(0, coordinate, limit);
        return coordinate;
    };
    const auto column = c.i8(player + 0x105);
    c.set(3, 0);
    c.set(0, std::uint32_t(column));
    c.compare(0, column, 2);
    const int minimum_row = attacking_away_goal ? 10 : 0;
    if (column >= 2 && compare_coordinate(0x105, 6) <= 6 &&
        compare_coordinate(0x106, minimum_row) >= minimum_row &&
        compare_coordinate(0x106, minimum_row + 1) <= minimum_row + 1)
        c.set(3, 1);
    c.set(0, c.r(3));
    c.compare(0, std::int32_t(c.r(0)), 0);
    return (c.cr(0) & 2) == 0;
}

bool in_shot_goal_area(Cpu& c, std::uint32_t player) {
    const auto side = c.u8(c.r(15) + 0x27);
    c.set(0, std::int8_t(side) == 0);
    c.compare(0, std::int32_t(c.r(0)), 1);
    const bool attacking_away_goal = c.cr(0) & 2;
    return in_goal_area_for_side(c, player, attacking_away_goal);
}

static bool draw_blocker_comment(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, c.u8(stack + 0x42));
    c.call(0x101ee530, 0x1025811c);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (c.cr(0) & 2) {
        c.set(0, std::uint32_t(c.i8(stack + 0x41)));
        c.compare(0, std::int32_t(c.r(0)), 0);
        if (c.cr(0) & 2) {
            const auto bound = c.i16(c.r(22) + 0x32);
            c.set(25, std::uint32_t(bound));
            draw_narrow_shot_sample(c, bound, 26, 0x10258164);
            c.compare(0, std::int32_t(c.r(26)), -10);
            if (!(c.cr(0) & 8)) return false;
        }
    }
    c.set(0, draw_shot_modulo_three(c, 0x102582c8));
    c.compare(0, std::int32_t(c.r(0)), 0);
    return (c.cr(0) & 2) == 0;
}

static void report_blocker(Cpu& c, std::uint32_t actor) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, 0x2168);
    c.set(7, c.u8(c.r(22) + 0x27));
    c.set(8, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(9, UINT32_MAX);
    c.set(6, c.u8(c.r(22) + 0x2f));
    c.set(10, 0);
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, 0x10258328);
}

void assess_shot_blocker(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    c.compare_logical(0, c.r(22), 0);
    if (c.cr(0) & 2) {
        c.s.pc = 0x102595e0;
        return;
    }
    c.set(3, std::uint32_t(c.i16(stack + 0x52)));
    c.set(0, std::uint32_t(c.i16(actor + 0x1a0)));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (c.cr(0) & 8) {
        c.set(0, 0);
        c.put8(stack + 0x40, 0);
    } else {
        c.set(3, c.u32(actor + 0x1a4));
        c.set(4, c.r(22));
        c.set(6, c.u8(stack + 0x42));
        c.set(5, actor);
        c.set({{8, stack + 0x4e}, {9, stack + 0x4d}, {10, stack + 0x41}, {7, UINT32_MAX}});
        c.call(0x101f5050, 0x10258040);
        c.put8(stack + 0x40, c.r(3));
        c.set(3, c.u32(actor + 0x1a4));
        c.set(0, std::uint32_t(c.i8(c.r(3) + 0x7931)));
        c.compare(0, std::int32_t(c.r(0)), 0);
        if (!(c.cr(0) & 2) && in_shot_goal_area(c, c.r(22)) && draw_blocker_comment(c, actor, stack))
            report_blocker(c, actor);
    }
    c.s.pc = 0x10258334;
}

static bool recognized_blocker_incident(Cpu& c, std::uint32_t actor) {
    c.set(3, c.u32(actor + 0x1a4));
    c.call(0x101e3d50, 0x102595a4);
    c.set(0, c.r(3) & 0x30);
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (c.cr(0) & 2) return false;
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, c.r(22));
    c.call(0x101e3d50, 0x102595b8);
    c.set(0, std::uint32_t(std::int16_t(c.r(3))) & 0x880);
    c.compare(0, std::int32_t(c.r(0)), 0);
    return (c.cr(0) & 2) == 0;
}

void finish_shot_blocker(Cpu& c) {
    const auto actor = c.r(15), blocker = c.r(22), stack = c.sp();
    if (c.s.pc == 0x102594a4) {
        c.set(0, std::uint32_t(c.i8(stack + 0x41)));
        c.compare(0, std::int32_t(c.r(0)), 0);
        if (!(c.cr(0) & 2)) {
            bool incident = in_shot_goal_area(c, actor);
            if (!incident) {
                c.set(0, std::uint32_t(c.i8(stack + 0x4c)));
                c.compare(0, std::int32_t(c.r(0)), 0);
                incident = !(c.cr(0) & 2);
            }
            if (incident) {
                c.set(0, 1);
                c.put8(stack + 0x121, 1);
            }
        }
    }
    const auto action = c.i8(stack + 0x42);
    c.set(4, blocker);
    c.set(0, std::uint32_t(action));
    c.put16(blocker + 0x19c, c.r(0));
    c.set(3, c.u8(blocker + 4));
    c.set(0, c.r(3) + 1);
    c.put8(blocker + 4, c.r(0));
    const unsigned penalty = recognized_blocker_incident(c, actor) ? 59 : 70;
    c.set(3, std::uint32_t(c.i16(blocker + 0x36)));
    c.set(0, c.r(3) - penalty);
    c.put16(blocker + 0x36, c.r(0));
    c.s.pc = 0x102595e0;
}
}
