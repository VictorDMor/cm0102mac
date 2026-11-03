#include "match_pl_goal_line.hpp"

namespace cm::aot::match_pl {
using namespace guest;

bool GoalLineAction::draw_keeper_pressure(std::int32_t bound, std::uint32_t back) {
    const unsigned result = first_register();
    if (compare(c, bound) == 0) c.set(result, 0);
    else {
        refill_pool(back);
        // The original's largest bound is a product of two unsigned bytes.
        c.set(3, std::uint32_t(-65535));
        compare(c, bound, -65535);
        c.set(3, c.u32(c.r(29)));
        compare(c, bound);
        c.set(0, c.u32(c.r(27)));
        c.set(3, c.u16(c.r(3)) + c.r(0));
        const auto remainder = std::uint32_t(std::int64_t(std::int32_t(c.r(3))) % bound);
        c.set(result, remainder);
        c.set(0, bound > 0 ? remainder : 0u - remainder);
        c.set(result, c.r(0));
    }
    return compare(c, std::int32_t(c.r(result))) != 0;
}

bool GoalLineAction::draw_keeper_trait(unsigned field, std::uint32_t back) {
    c.set(3, c.u32(c.r(30) + 0x6c));
    c.set(0, c.u8(c.r(3) + field));
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    c.s.wr<1>(reg::xer_ca, c.r(3) == UINT32_MAX);
    const auto bound = std::int32_t(c.r(3) + 1);
    c.set(opposite ? 20 : 19, std::uint32_t(bound));
    return draw_keeper_pressure(bound, back);
}

void GoalLineAction::refresh_keeper_after_goal() {
    c.set(3, c.u8(c.r(30) + 0xf));
    c.set(0, c.u8(c.r(30) + 0xf));
    const auto bound = std::int32_t(c.r(3) * c.r(0));
    c.set(opposite ? 20 : 19, std::uint32_t(bound));
    // The next original compare replaces CR0 before any external call.
    draw_keeper_pressure(bound, resume(0x1026b0f0, 0x1026e474));
    c.set(0, c.u8(actor.address + 0xf));
    c.set(3, std::uint32_t(std::int8_t(c.r(first_register()))));
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) < 0) {
        c.set(0, c.u8(actor.address + 0xf));
        c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    }
    c.set(5, c.u8(c.r(30) + 0x103));
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    c.set({{3, c.r(30)}, {4, 0}});
    c.set(0, c.r(5) + c.r(0));
    c.put8(c.r(30) + 0x103, c.r(0));
    c.call(0x1022ba10, resume(0x1026b260, 0x1026e5e4));
}

void GoalLineAction::finish_pending_restart(std::uint32_t back) {
    c.set(3, actor.match());
    c.call(0x101d6980, back);
    c.set(0, c.r(3));
    c.set(3, actor.match());
    c.set(first_register(), c.r(0));
    c.call(0x101d5980, back + 0x10);
    c.set(0, std::uint32_t(std::int8_t(actor.side())));
    c.set(3, std::uint32_t(std::int8_t(c.r(first_register()))));
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) == 0) {
        announce(0x20df, actor.address, false, back + 0x4c, false);
        c.set(3, std::uint32_t(c.i8(frame.at(0x3d))));
        c.set(0, std::uint32_t(c.i8(frame.at(0x3e))));
        c.set(0, c.r(0) - c.r(3));
        c.s.wr<1>(reg::xer_ca, c.r(0) <= 3);
        const auto bound = std::int32_t(3 - c.r(0));
        c.set(first_register(), std::uint32_t(bound));
        const unsigned result = opposite ? 20 : 19;
        if (compare(c, bound) == 0) c.set(result, 0);
        else {
            refill_pool(back + 0x94);
            // This bound is 3 minus the difference of two signed tally bytes.
            c.set(3, std::uint32_t(-65535));
            compare(c, bound, -65535);
            c.set(3, c.u32(c.r(29)));
            compare(c, bound);
            c.set(0, c.u32(c.r(27)));
            c.set(3, c.u16(c.r(3)) + c.r(0));
            const auto remainder = std::uint32_t(std::int64_t(std::int32_t(c.r(3))) % bound);
            c.set(result, remainder);
            c.set(0, bound > 0 ? remainder : 0u - remainder);
            c.set(result, c.r(0));
        }
        if (compare(c, std::int32_t(c.r(result))) > 0)
            announce(0x20d9, actor.address, false, back + 0x200, false);
        c.set({{3, actor.match()}, {4, actor.address}});
        c.set_f(1, c.f32(c.r(floats_register()) + 0xe4));
        c.call(0x101d6780, back + 0x210);
    } else {
        if ((score_matches(0) || score_matches(1)) && draw_constant_zero(back + 0x268, 40, 0x66666667, 4))
            announce(0x20d9, actor.address, false, back + 0x2cc, false);
        c.set({{3, actor.match()}, {4, actor.address}});
        c.set_f(1, c.f32(c.r(floats_register()) + 0xa0));
        c.call(0x101d6780, back + 0x2dc);
    }
    c.set({{3, actor.address}, {4, 0}});
    c.call(0x102884d0, back + 0x2e8);
}

void GoalLineAction::prepare_awarded_goal() {
    c.set(3, actor.match());
    c.call(0x101d6980, resume(0x1026ad1c, 0x1026e0f4));
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0)), -1) != 0) return;
    c.set(3, actor.match());
    c.call(0x101fcde0, resume(0x1026ad30, 0x1026e108));
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) != 0) {
        c.set(3, actor.match());
        c.set(0, 1);
        c.put8(c.r(3) + 0x1c1, c.r(0));
    }
    if (opposite) {
        c.set(4, c.u8(actor.address + 0xc));
        c.set(3, c.r(30));
        c.set(0, c.r(4) + 1);
        c.put8(actor.address + 0xc, c.r(0));
        for (unsigned field : {0x3eu, 0x40u, 0x42u, 0x44u, 0x46u})
            modify_halfword(actor.address + field, 1, 4);
        modify_halfword(actor.address + 0x36, 137, 4);
    }
    if (!keeper_role(resume(0x1026ada0, 0x1026e124))) return;
    c.set(3, c.u8(c.r(30) + 0xf));
    c.set(0, c.r(3) + 1);
    c.put8(c.r(30) + 0xf, c.r(0));
    if (draw_keeper_trait(0x64, resume(0x1026adf4, 0x1026e178)) &&
        draw_keeper_trait(0x66, resume(0x1026af74, 0x1026e2f8))) refresh_keeper_after_goal();
    modify_halfword(c.r(30) + 0x36, -500);
    if (stack_byte(0x38)) modify_halfword(c.r(30) + 0x36, -600);
}

bool GoalLineAction::review_initial_goal() {
    bool check_recovery = opposite;
    if (!opposite) {
        check_recovery = keeper_role(0x1026e610);
        if (check_recovery) check_recovery = compare_logical(c, c.r(30), actor.address) != 0;
    }
    if (check_recovery && !stack_byte(0x38) &&
        draw_constant_zero(resume(0x1026b2b0, 0x1026e650), 50, 0x51eb851f, 4) &&
        !stack_byte(0x39) && (!opposite || keeper_role(0x1026b300)))
        announce(0x20d6, c.r(30), false, resume(0x1026b334, 0x1026e6c0));
    announce(0x1fff, actor.address, !opposite, resume(0x1026b35c, 0x1026e6f8));
    if (draw_constant_zero(resume(0x1026b37c, 0x1026e718), 3, 0x55555556, 0)) return false;
    c.set(3, actor.match());
    c.call(0x101d6980, resume(0x1026b3bc, 0x1026e758));
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0)), -1) <= 0) return false;
    announce(0x20ed, actor.address, !opposite, resume(0x1026b3f0, 0x1026e79c), false);
    finish_pending_restart(resume(0x1026b3f8, 0x1026e7a4));
    return true;
}

bool GoalLineAction::pending_restart(std::uint32_t back) {
    c.set(3, actor.match());
    c.call(0x101d6980, back);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    return compare(c, std::int32_t(c.r(0)), -1) > 0;
}

bool GoalLineAction::review_later_goal() {
    const bool pending = pending_restart(resume(0x1026b6ec, 0x1026ea98));
    if (pending) announce(resume(0x2115, 0x20fa), actor.address, false, resume(0x1026b720, 0x1026ead0));
    else announce(resume(0x2114, 0x1fcd), actor.address, false, resume(0x1026b74c, 0x1026eb00));
    if (!opposite || !draw_constant_zero(0x1026b76c, 3, 0x55555556, 0)) {
        if (pending_restart(resume(0x1026b7ac, 0x1026eb08))) {
            announce(0x20ed, actor.address, false, resume(0x1026b7e0, 0x1026eb3c), false);
            finish_pending_restart(resume(0x1026b7e8, 0x1026eb44));
            return true;
        }
    }
    if (!opposite) return false;
    c.set(0, c.u8(actor.address + 0xc));
    if (compare_logical(c, c.r(0), 3) == 0) {
        c.set(0, std::uint32_t(c.i8(actor.address + 0x31)));
        if (compare(c, std::int32_t(c.r(0))) != 0) announce(0x2011, actor.address, false, 0x1026bb14);
        else announce(0x1fd5, actor.address, false, 0x1026bb40);
    }
    if (!pending_restart(0x1026bb48)) return false;
    announce(0x20ed, actor.address, false, 0x1026bb7c, false);
    finish_pending_restart(0x1026bb84);
    return true;
}
}
