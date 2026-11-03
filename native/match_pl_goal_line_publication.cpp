#include "match_pl_goal_line.hpp"

namespace cm::aot::match_pl {
using namespace guest;

std::uint32_t GoalLineAction::live_goal_club() {
    c.set(0, std::uint32_t(std::int8_t(actor.side())));
    c.set(3, actor.match());
    if (!opposite) c.set(0, c.r(0) == 0);
    const bool second = compare(c, std::int32_t(c.r(0))) != 0;
    c.set(3, c.u32(c.r(3) + 0x3118));
    c.set(3, c.u32(c.r(3) + (second ? 0x20 : 0x1c)));
    return c.r(3);
}

bool GoalLineAction::goal_club_uses_comment() {
    c.set(0, c.u32(live_goal_club() + 0x2c));
    if (compare_logical(c, c.r(0)) == 0) return false;
    c.set(3, c.u32(live_goal_club() + 0x2c));
    c.set(0, c.u8(c.r(3) + 0x119));
    return compare(c, std::int32_t(c.r(0)), 8) == 0;
}

bool GoalLineAction::announce_goal_celebration() {
    refill_pool(resume(0x1026c8b8, 0x1026f51c));
    c.set(3, c.u32(c.r(29)));
    c.set(4, 0x66660000);
    c.set(5, c.u32(c.r(27)));
    c.set(4, 0x66666667);
    c.set(6, c.u16(c.r(3)));
    c.set(0, c.u8(frame.at(0x3d)));
    c.set(5, c.r(6) + c.r(5));
    const auto high = std::int32_t((std::int64_t(std::int32_t(c.r(5))) * 0x66666667) >> 32);
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    c.set(0, std::uint32_t(c.i8(frame.at(0x3e))));
    c.set(3, c.r(0) - c.r(3));
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & 1));
    const auto quotient = high >> 1;
    c.set(4, c.r(5) - 5 * std::uint32_t(quotient + (quotient < 0)));
    c.set(0, c.r(4) + 2);
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) <= 0 ||
        !draw_constant_zero(resume(0x1026c930, 0x1026f594), 7, std::int32_t(0x92492493u), 2) ||
        !goal_club_uses_comment()) return false;
    announce(0x2117, actor.address, !opposite, resume(0x1026c9fc, 0x1026f688), false);
    return true;
}

void GoalLineAction::announce_late_goal() {
    if (!score_matches(0) && !score_matches(1) && !stack_byte(0x3c)) return;
    const auto sample_register = opposite ? 19u : 18u;
    c.set(4, actor.match());
    const auto ticks = c.i16(c.r(4) + 0x7948);
    c.set(first_register(), std::uint32_t(ticks));
    if (compare(c, match_minute(ticks)) == 0) c.set(sample_register, 0);
    else {
        refill_pool(resume(0x1026ca90, 0x1026f71c));
        // The original divided a sign-extended halfword before checking ±65,535.
        const auto high = std::int32_t((std::int64_t(ticks) * 0x2e8ba2e9) >> 32);
        c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & 1));
        const auto quotient = high >> 1;
        const auto bound = quotient + (quotient < 0);
        c.set(4, 0x2e8ba2e9);
        c.set(3, std::uint32_t(-65535));
        c.set(0, std::uint32_t(bound));
        compare(c, bound, -65535);
        c.set(5, c.u32(c.r(29)));
        c.set(6, c.u16(c.r(5)));
        c.set(5, c.u32(c.r(27)));
        c.set(6, c.r(6) + c.r(5));
        c.set(5, std::uint32_t(bound));
        const auto value = std::int32_t(c.r(6));
        const auto remainder = std::uint32_t(std::int64_t(value) % bound);
        c.set(4, c.r(6) - remainder);
        c.set(3, unsigned(quotient < 0));
        compare(c, bound);
        c.set(0, bound > 0 ? remainder : 0u - remainder);
        c.set(sample_register, c.r(0));
    }
    const auto rival = draw_constant(resume(0x1026ccc8, 0x1026f954), 120, std::int32_t(0x88888889u), 6);
    if (compare(c, std::int32_t(c.r(sample_register)), rival) > 0 && goal_club_uses_comment())
        announce(0x2116, actor.address, !opposite, resume(0x1026cd98, 0x1026fa4c), false);
}

void GoalLineAction::finish_confirmed_goal() {
    bool comment_on_keeper;
    if (opposite) comment_on_keeper = stack_byte(0x38) && !stack_byte(0x39) && keeper_role(0x1026c830);
    else comment_on_keeper = keeper_role(0x1026f468) &&
        compare_logical(c, c.r(30), actor.address) != 0 && stack_byte(0x38) && !stack_byte(0x39);
    if (comment_on_keeper) announce(0x20d5, c.r(30), false, resume(0x1026c864, 0x1026f4bc));
    c.set({{3, actor.match()}, {4, actor.address}});
    c.set_f(1, c.f32(c.r(floats_register()) + resume(0x48, 0x13c)));
    c.call(0x101d6780, resume(0x1026c874, 0x1026f4cc));

    c.set(0, std::uint32_t(std::int8_t(actor.side())));
    c.set(3, actor.match() + 0x10000);
    if (!opposite) c.set(0, c.r(0) == 0);
    c.set(0, c.r(0) * 0x36);
    c.set(4, c.r(3) + c.r(0));
    c.set(3, c.u8(c.r(4) - 0x1fc3));
    c.set(0, c.r(3) + 1);
    c.put8(c.r(4) - 0x1fc3, c.r(0));
    if (!announce_goal_celebration()) announce_late_goal();

    if (opposite) {
        c.set({{0, actor.side()}, {5, 0}, {6, actor.match()}, {3, 8}});
        c.set(4, std::int8_t(c.r(0)) == 0);
        c.set(0, 0);
        c.put8(c.r(6) + 0x7924, c.r(4));
    } else {
        c.set({{6, actor.side()}, {5, 0}, {4, actor.match()}, {3, 8}, {0, 0}});
        c.put8(c.r(4) + 0x7924, c.r(6));
    }
    c.set(4, actor.match() + 0x10000);
    c.put32(c.r(4) - 0x2000, c.r(5));
    c.set(4, actor.match());
    c.put8(c.r(4) + 0x7928, c.r(3));
    c.set(3, actor.match());
    c.put8(c.r(3) + 0x1e3, c.r(0));
}
}
