#include "match_pl_shot_event_detail.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

namespace {
int read_signed(Cpu& c, std::int32_t value, std::int32_t bound = 0) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, bound);
    return value < bound ? -1 : value > bound ? 1 : 0;
}

bool has_keeper_role(Cpu& c, std::uint32_t keeper, std::uint32_t back) {
    c.set(3, keeper);
    call_shot_owner(c, 0x10289560, back);
    return read_signed(c, std::int16_t(c.r(3)), 1) == 0;
}

void report_count_milestone(Cpu& c, std::uint32_t actor, unsigned field, unsigned event, std::uint32_t back) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, event);
    c.set(10, std::uint32_t(c.i16(actor + field)));
    c.set(8, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(9, UINT32_MAX);
    c.set(7, c.u8(actor + 0x27));
    c.set(6, c.u8(actor + 0x2f));
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, back);
}

bool is_count_multiple(Cpu& c, std::uint32_t actor, unsigned field, unsigned divisor) {
    const auto shift = divisor == 100 ? 5 : divisor == 10 ? 2 : 1;
    const auto multiplier = divisor == 100 ? 0x51eb851f : 0x66666667;
    c.set(3, divisor == 100 ? 0x51ec0000 : 0x66660000);
    c.set(4, std::uint32_t(c.i16(actor + field)));
    c.set(3, multiplier);
    const auto high = std::int32_t((std::int64_t(std::int32_t(c.r(4))) * multiplier) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << shift) - 1)));
    const auto shifted = high >> shift;
    c.set(3, shifted < 0);
    return read_signed(c, std::int32_t(c.r(4)) - (shifted + std::int32_t(c.r(3))) * int(divisor)) == 0;
}

bool is_hundred_milestone(Cpu& c, std::uint32_t actor, unsigned field) {
    return read_signed(c, c.i16(actor + field), 50) == 0 ||
           (read_signed(c, c.i16(actor + field), 100) >= 0 && is_count_multiple(c, actor, field, 100));
}

bool changed_goal_counter(Cpu& c, std::uint32_t actor, unsigned field, unsigned earlier) {
    c.set(3, std::uint32_t(c.i16(actor + field)));
    c.set(0, std::uint32_t(c.i16(actor + earlier)));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    return c.r(3) != c.r(0);
}

void report_scorer_milestone(Cpu& c, std::uint32_t actor) {
    if (read_signed(c, c.i8(actor + 0x31)) != 0) {
        c.set(0, c.u8(actor + 0xc));
        c.compare_logical(0, c.r(0), 1);
        if (c.cr(0) & 2) {
            c.set({{3, actor}, {4, 0x21b6}});
            call_shot_owner(c, 0x10289d50, 0x10265a2c);
            const bool enabled = read_signed(c, std::int8_t(c.r(3))) != 0;
            report_count_milestone(c, actor, 0x3e, enabled ? 0x21b6 : 0x2010,
                                   enabled ? 0x10265a5c : 0x10265a88);
            return;
        }
    }
    if (read_signed(c, c.i16(actor + 0x3e), 1) == 0) {
        report_count_milestone(c, actor, 0x3e, 0x200f, 0x10265ac0);
    } else if (read_signed(c, c.i16(actor + 0x40), 1) == 0) {
        report_count_milestone(c, actor, 0x40, 0x200b, 0x10265af8);
    } else if (read_signed(c, c.i16(actor + 0x44), 1) == 0) {
        report_count_milestone(c, actor, 0x44, 0x2012, 0x10265b30);
    } else if (read_signed(c, c.i16(actor + 0x40), 10) >= 0 && is_count_multiple(c, actor, 0x40, 10)) {
        report_count_milestone(c, actor, 0x40, 0x200b, 0x10265b90);
    } else if (is_hundred_milestone(c, actor, 0x3e)) {
        report_count_milestone(c, actor, 0x3e, 0x200a, 0x10265bfc);
    } else if (changed_goal_counter(c, actor, 0x42, 0x3e) && is_hundred_milestone(c, actor, 0x42)) {
        report_count_milestone(c, actor, 0x42, 0x200c, 0x10265c78);
    } else if (changed_goal_counter(c, actor, 0x44, 0x42) &&
               read_signed(c, c.i16(actor + 0x44), 100) >= 0 && is_count_multiple(c, actor, 0x44, 100)) {
        report_count_milestone(c, actor, 0x44, 0x200d, 0x10265ce8);
    } else if (read_signed(c, c.i16(actor + 0x46), 1) == 0 ||
               (read_signed(c, c.i16(actor + 0x46), 5) >= 0 && is_count_multiple(c, actor, 0x46, 5))) {
        report_count_milestone(c, actor, 0x46, 0x200e, 0x10265d54);
    }
}

void report_optional_scorer_event(Cpu& c, std::uint32_t actor) {
    unsigned back = 0x10265d60;
    for (const auto event : {0x219fu, 0x21a0u, 0x21a1u, 0x21b0u, 0x21b3u, 0x21b4u, 0x21b5u}) {
        c.set({{3, actor}, {4, event}});
        call_shot_owner(c, 0x10289d50, back);
        if (read_signed(c, std::int8_t(c.r(3))) != 0) {
            report_shot_event(c, actor, event, back + 0x30);
            return;
        }
        back += 0x40;
    }
}
}

void describe_goal_distance(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    if (read_signed(c, c.i8(stack + 0x128)) == 0 && read_signed(c, c.i8(stack + 0x127), 20) >= 0) {
        if (read_signed(c, c.i8(stack + 0x127), 40) > 0) c.set(4, 0x1fa1);
        else if (read_signed(c, c.i8(stack + 0x127), 25) > 0) c.set(4, 0x1fa0);
        else c.set(4, 0x1f9f);
        c.set(3, c.u32(actor + 0x1a4));
        c.set(8, UINT32_MAX);
        c.set(0, c.u8(stack + 0x127));
        c.set(9, UINT32_MAX);
        c.set(7, c.u8(actor + 0x27));
        c.set(3, c.u32(c.r(3) + 0x79f4));
        c.set(10, std::uint32_t(std::int8_t(c.r(0))));
        c.set(6, c.u8(actor + 0x2f));
        c.set(5, c.u32(actor + 0x1a4));
        c.call(0x10201180, 0x10265618);
    }
    c.s.pc = 0x10265618;
}

void describe_goal_milestones(Cpu& c) {
    const auto actor = c.r(15);
    if (read_signed(c, c.i8(c.sp() + 0x128)) == 0) {
        report_scorer_milestone(c, actor);
        report_optional_scorer_event(c, actor);
    }
    c.s.pc = 0x10265f10;
}

void describe_goal_keeper_note(Cpu& c) {
    const auto actor = c.r(15), keeper = c.r(24), stack = c.sp();
    c.set(3, c.u32(actor + 0x1a4));
    if (read_signed(c, c.i8(c.r(3) + 0x1bd)) == 0 && has_keeper_role(c, keeper, 0x1026630c) &&
        read_signed(c, c.i8(stack + 0x46)) != 0) {
        c.set(3, c.u32(actor + 0x1a4));
        c.set(4, 0x1fe1);
        c.set(10, c.u8(stack + 0x46));
        c.set(8, UINT32_MAX);
        c.set(3, c.u32(c.r(3) + 0x79f4));
        c.set(9, UINT32_MAX);
        c.set(7, c.u8(keeper + 0x27));
        c.set(10, std::uint32_t(std::int8_t(c.r(10))));
        c.set(6, c.u8(keeper + 0x2f));
        c.set(5, c.u32(actor + 0x1a4));
        c.call(0x10201180, 0x10266350);
    }
    c.s.pc = 0x10266350;
}

void select_goal_confirmation(Cpu& c) {
    const auto actor = c.r(15), keeper = c.r(24), stack = c.sp();
    if (read_signed(c, c.i8(stack + 0x45)) != 0 && read_signed(c, c.i8(stack + 0x46)) == 0 &&
        has_keeper_role(c, keeper, 0x10267114)) {
        c.set(3, c.u32(actor + 0x1a4));
        c.set(4, 0x20d5);
        c.set(7, c.u8(keeper + 0x27));
        c.set(8, UINT32_MAX);
        c.set(3, c.u32(c.r(3) + 0x79f4));
        c.set(9, UINT32_MAX);
        c.set(6, c.u8(keeper + 0x2f));
        c.set(10, 0);
        c.set(5, c.u32(actor + 0x1a4));
        c.call(0x10201180, 0x10267148);
    }
    c.s.pc = read_signed(c, std::int8_t(c.r(17)), 1) == 0 ? 0x10267154 : 0x10267b28;
}
}
