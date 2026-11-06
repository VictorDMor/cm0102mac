#include "match_pl_shot_event_detail.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

namespace {
bool above(Cpu& c, std::int32_t value, std::int32_t limit = 0) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, limit);
    return (c.cr(0) & 4) != 0;
}

bool nonzero(Cpu& c, std::int32_t value) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, 0);
    return value != 0;
}

bool above_unsigned(Cpu& c, unsigned value, unsigned limit) {
    c.set(0, value);
    c.compare_logical(0, value, limit);
    return (c.cr(0) & 4) != 0;
}

bool has_roles(Cpu& c, std::uint32_t player, unsigned mask, std::uint32_t back) {
    c.set(3, player);
    call_shot_owner(c, 0x10289560, back);
    return nonzero(c, std::int32_t(c.r(3) & mask));
}

bool has_both_marking_roles(Cpu& c, std::uint32_t player, std::uint32_t back) {
    c.set(3, player);
    call_shot_owner(c, 0x1028abc0, back);
    return nonzero(c, std::int8_t(c.r(3)));
}

void report_role_event(Cpu& c, std::uint32_t scorer, std::uint32_t subject, unsigned event, std::uint32_t back) {
    c.set(3, c.u32(scorer + 0x1a4));
    c.set(4, event);
    c.set(7, c.u8(subject + 0x27));
    c.set(8, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(9, UINT32_MAX);
    c.set(6, c.u8(subject + 0x2f));
    c.set(10, 0);
    c.set(5, c.u32(scorer + 0x1a4));
    c.call(0x10201180, back);
}

bool is_scoring_assistant_context(Cpu& c, std::uint32_t actor) {
    c.set(3, c.u32(actor + 0x1a4) + 0x10000);
    c.set(0, c.u32(c.r(3) - 0x1fd8));
    c.compare_logical(0, actor, c.r(0));
    if (!(c.cr(0) & 2)) return false;
    c.set(3, c.u32(actor + 0x1a4) + 0x10000);
    c.set(0, c.u32(c.r(3) - 0x1fdc));
    c.compare_logical(0, c.r(0), 0);
    if (!c.r(0)) return false;
    c.set(3, c.u32(actor + 0x1a4) + 0x10000);
    c.set(14, c.u32(c.r(3) - 0x1fdc));
    return true;
}

bool is_qualifying_assist_action(Cpu& c, std::uint32_t actor) {
    for (const auto accepted : {94, 6, 7, 8}) {
        c.set(3, c.u32(actor + 0x1a4));
        const auto action = c.i16(c.r(3) + 0x77c0);
        c.set(0, std::uint32_t(action));
        c.compare(0, action, accepted);
        if (action == accepted) return true;
    }
    return false;
}

bool draws_assistant_note(Cpu& c) {
    advance_shot_stream(c, 0x102643a0);
    c.set(3, c.u32(c.r(31)));
    c.set(0, c.u32(c.r(28)));
    const auto sample = c.u16(c.r(3)) + c.r(0);
    const auto sign = sample >> 31;
    c.set(3, sign);
    return nonzero(c, std::int32_t(((sample & 1) ^ sign) - sign));
}

void describe_assistant_role(Cpu& c, std::uint32_t actor) {
    const auto assistant = c.r(14);
    if (has_both_marking_roles(c, assistant, 0x10264220) &&
        has_roles(c, assistant, 0x38, 0x10264230) && is_qualifying_assist_action(c, actor)) {
        if (above(c, c.i16(assistant + 0x36), 6750) &&
            above_unsigned(c, c.u8(assistant + 3), 1) &&
            above_unsigned(c, c.u8(assistant + 0x10), 1) &&
            above_unsigned(c, c.u8(assistant + 1), 10)) {
            report_role_event(c, actor, assistant, 0x21c8, 0x102642d0);
        } else if (above(c, c.i16(assistant + 0x36), 6750) &&
                   above_unsigned(c, c.u8(assistant + 1), 10) &&
                   above_unsigned(c, c.u8(assistant + 3), 1) &&
                   nonzero(c, c.i8(assistant + 0x10e))) {
            report_role_event(c, actor, assistant, 0x21ca, 0x1026432c);
        }
    } else if (!has_both_marking_roles(c, assistant, 0x10264338) &&
               has_roles(c, assistant, 0x30, 0x10264348) &&
               above_unsigned(c, c.u8(assistant + 2), 10) &&
               above(c, c.i16(assistant + 0x36), 6750) &&
               above_unsigned(c, c.u8(assistant + 3), 1) &&
               above_unsigned(c, c.u8(assistant + 1), 10) &&
               draws_assistant_note(c) && nonzero(c, c.i8(assistant + 0x10e))) {
        report_role_event(c, actor, assistant, 0x21c9, 0x10264400);
    }
}

std::int32_t signed_sample_quotient(Cpu& c, std::uint32_t sample, unsigned shift) {
    const auto high = std::int32_t((std::int64_t(std::int32_t(sample)) * 0x66666667) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << shift) - 1)));
    const auto shifted = high >> shift;
    return shifted + (shifted < 0);
}

bool keeper_role(Cpu& c, std::uint32_t keeper, std::uint32_t back) {
    c.set(3, keeper);
    call_shot_owner(c, 0x10289560, back);
    const auto role = std::int16_t(c.r(3));
    c.set(0, std::uint32_t(role));
    c.compare(0, role, 1);
    return role == 1;
}

bool keeper_skill_roll(Cpu& c, std::uint32_t keeper, unsigned pointer, unsigned trait, std::uint32_t back) {
    advance_shot_stream(c, back);
    c.set(5, c.u32(c.r(31)));
    c.set(4, 0x66660000);
    c.set(3, c.u32(keeper + pointer));
    c.set(4, 0x66666667);
    c.set(5, c.u16(c.r(5)));
    c.set(0, c.u32(c.r(28)));
    c.set(3, c.u8(c.r(3) + trait));
    const auto sample = c.r(5) + c.r(0);
    c.set(5, sample);
    c.set(0, c.u8(keeper + 0xf));
    c.set(3, std::uint32_t(std::int8_t(c.r(3))) + c.r(0));
    const auto remainder = sample - std::uint32_t(signed_sample_quotient(c, sample, 2)) * 10;
    c.set(4, remainder);
    c.set(0, std::uint32_t(std::int8_t(remainder + 5)));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    return (c.cr(0) & 4) != 0;
}

bool keeper_commentary_draw(Cpu& c, std::uint32_t keeper, std::uint32_t back) {
    const auto bound = c.u8(keeper + 0xf);
    c.set(16, bound);
    draw_narrow_shot_sample(c, bound, 14, back);
    c.compare(0, std::int32_t(c.r(14)), 0);
    return c.r(14) != 0;
}

bool keeper_praise_roll(Cpu& c, std::uint32_t actor, std::uint32_t keeper) {
    advance_shot_stream(c, 0x102646c8);
    c.set(3, c.u32(c.r(31)));
    c.set(4, 0x66660000);
    c.set(5, c.u32(c.r(28)));
    c.set(4, 0x66666667);
    c.set(6, c.u16(c.r(3)));
    c.set(0, c.u8(actor + 0x103));
    const auto sample = c.r(6) + c.r(5);
    c.set(5, sample);
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    c.set(0, c.u8(keeper + 0xf));
    c.set(3, c.r(3) + c.r(0));
    const auto remainder = sample - std::uint32_t(signed_sample_quotient(c, sample, 2)) * 10;
    c.set(4, remainder);
    c.set(0, std::uint32_t(std::int8_t(remainder + 1)));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    return (c.cr(0) & 4) != 0;
}

bool describe_strong_keeper(Cpu& c, std::uint32_t actor, std::uint32_t keeper, std::uint32_t stack) {
    if (nonzero(c, c.i8(stack + 0x45)) || nonzero(c, c.i8(stack + 0x46))) return false;
    c.compare_logical(0, keeper, 0);
    if (!keeper || !keeper_role(c, keeper, 0x1026442c) ||
        !keeper_skill_roll(c, keeper, 0x70, 0x1c, 0x10264458) ||
        !keeper_skill_roll(c, keeper, 0x6c, 0x63, 0x102644d4) ||
        !keeper_commentary_draw(c, keeper, 0x10264564) || !keeper_praise_roll(c, actor, keeper)) return false;
    report_role_event(c, actor, keeper, 0x21d3, 0x10264748);
    return true;
}

bool keeper_criticism_roll(Cpu& c, std::uint32_t keeper) {
    advance_shot_stream(c, 0x102647d0);
    c.set(3, c.u32(c.r(31)));
    c.set(5, 0x66660000);
    c.set(4, c.u32(keeper + 0x6c));
    c.set(5, 0x66666667);
    c.set(6, c.u16(c.r(3)));
    c.set(0, c.u32(c.r(28)));
    c.set(4, c.u8(c.r(4) + 0x66));
    c.set(7, c.r(6) + c.r(0));
    c.set(3, c.u8(keeper + 0xf));
    c.set(4, std::uint32_t(std::int8_t(c.r(4))));
    c.set(0, c.r(3) + 4);
    c.set(3, c.u8(keeper + 0xf));
    const auto quotient = signed_sample_quotient(c, c.r(7), 1);
    // The sign is taken from the arithmetic-shift result before truncation.
    const auto high = std::int32_t((std::int64_t(std::int32_t(c.r(7))) * 0x66666667) >> 32);
    c.set(6, high < 0);
    c.set(5, c.r(7) - std::uint32_t(quotient) * 5);
    c.set(0, c.r(3) + c.r(0) + c.r(5));
    c.compare(0, std::int32_t(c.r(4)), std::int32_t(c.r(0)));
    return (c.cr(0) & 8) != 0;
}

bool draws_keeper_criticism(Cpu& c, std::uint32_t actor) {
    advance_shot_stream(c, 0x102649d4);
    c.set(3, c.u32(c.r(31)));
    c.set(4, 0x66660000);
    c.set(5, c.u32(c.r(28)));
    c.set(4, 0x66666667);
    c.set(6, c.u16(c.r(3)));
    c.set(0, c.u8(actor + 0x103));
    const auto sample = c.r(6) + c.r(5);
    c.set(5, sample);
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    c.set(4, sample - std::uint32_t(signed_sample_quotient(c, sample, 1)) * 5);
    c.set(0, std::uint32_t(std::int8_t(c.r(4) + 1)));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    return (c.cr(0) & 8) != 0;
}

void describe_weak_keeper(Cpu& c, std::uint32_t actor, std::uint32_t keeper, std::uint32_t stack) {
    c.compare_logical(0, keeper, 0);
    if (!keeper || nonzero(c, c.i8(stack + 0x45)) || nonzero(c, c.i8(stack + 0x46)) ||
        !keeper_role(c, keeper, 0x10264774)) return;
    c.set(3, c.u32(keeper + 0x70));
    const auto ability = c.i8(c.r(3) + 0x1c);
    c.set(0, std::uint32_t(ability));
    c.compare(0, ability, 10);
    if (!(c.cr(0) & 8)) return;
    c.set(4, c.u32(keeper + 0x6c));
    c.set(3, c.u8(keeper + 0xf));
    c.set(4, c.u8(c.r(4) + 0x63));
    c.set(0, c.r(3) + 10);
    c.set(3, std::uint32_t(std::int8_t(c.r(4))));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (!(c.cr(0) & 8) || !keeper_criticism_roll(c, keeper) ||
        !above_unsigned(c, c.u8(keeper + 0xf), 1) ||
        !keeper_commentary_draw(c, keeper, 0x10264870) || !draws_keeper_criticism(c, actor)) return;
    report_role_event(c, actor, keeper, 0x21d4, 0x10264a4c);
}
}

void describe_goal_roles(Cpu& c) {
    const auto actor = c.r(15), keeper = c.r(24), stack = c.sp();
    if (has_roles(c, actor, 0x30, 0x10264188) && !has_both_marking_roles(c, actor, 0x10264198) &&
        above(c, c.i16(actor + 0x36), 6750) && above_unsigned(c, c.u8(actor + 0xb), 1)) {
        report_shot_event(c, actor, 0x21c7, 0x102641e0);
    } else if (is_scoring_assistant_context(c, actor)) {
        describe_assistant_role(c, actor);
    } else if (!describe_strong_keeper(c, actor, keeper, stack)) {
        describe_weak_keeper(c, actor, keeper, stack);
    }
    c.s.pc = 0x10264bb0;
}
}
