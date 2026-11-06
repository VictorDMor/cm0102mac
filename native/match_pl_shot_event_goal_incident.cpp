#include "match_pl_shot_event_detail.hpp"

#include <array>

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

namespace {
constexpr auto incident_exit = 0x1026a190;

void report_incident(Cpu& c, std::uint32_t actor, unsigned event, std::uint32_t back) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, event);
    c.set(7, c.u8(actor + 0x27));
    c.set(6, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(8, UINT32_MAX);
    c.set(5, c.u32(actor + 0x1a4));
    c.set({{9, UINT32_MAX}, {10, 0}});
    c.call(0x10201180, back);
}

void update_incident(Cpu& c, std::uint32_t actor, unsigned weight_offset, std::uint32_t back) {
    c.set(5, c.toc(-0x51e0));
    c.set(4, actor);
    c.set(3, c.u32(actor + 0x1a4));
    c.set_f(1, c.f32(c.r(5) + weight_offset));
    c.call(0x101d6780, back);
}

bool condition_from_goal_difference(Cpu& c, std::uint32_t stack, std::uint32_t refill_return) {
    c.set(3, c.u8(stack + 0x4a));
    c.set(0, c.u8(stack + 0x4b));
    c.set(3, std::uint32_t(std::int8_t(c.r(3))));
    const auto difference = std::uint32_t(std::int8_t(c.r(0))) - c.r(3);
    c.set(0, difference);
    const auto bound = std::int32_t(3u - difference);
    c.s.wr<1>(reg::xer_ca, difference <= 3);
    c.set(13, std::uint32_t(bound));
    // Both score operands are signed bytes, keeping the bound in [-252, 258].
    draw_narrow_shot_sample(c, bound, 14, refill_return);
    c.compare(0, std::int32_t(c.r(14)), 0);
    return (c.cr(0) & 4) != 0;
}

bool equal_or_one_ahead(Cpu& c, std::uint32_t stack) {
    c.set(3, c.u8(stack + 0x4b));
    c.set(0, c.u8(stack + 0x4a));
    c.set(3, std::uint32_t(std::int8_t(c.r(3))));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (c.cr(0) & 2) return true;
    c.set(3, c.u8(stack + 0x4b));
    c.set(0, c.u8(stack + 0x4a));
    c.set(3, std::uint32_t(std::int8_t(c.r(3)) - 1));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    return (c.cr(0) & 2) != 0;
}

bool draws_opposing_incident(Cpu& c, std::uint32_t refill_return) {
    advance_shot_stream(c, refill_return);
    c.set(4, c.u32(c.r(31)));
    c.set(3, 0x66660000);
    c.set(0, c.u32(c.r(28)));
    c.set(3, 0x66666667);
    const auto sample = c.u16(c.r(4)) + c.r(0);
    c.set(4, sample);
    const auto high = std::int32_t((std::int64_t(std::int32_t(sample)) * 0x66666667) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (high & 15));
    const auto shifted = std::uint32_t(high >> 4);
    c.set(3, shifted >> 31);
    c.set(0, sample - (shifted + c.r(3)) * 40);
    c.compare(0, std::int32_t(c.r(0)), 0);
    return c.r(0) == 0;
}

void rollback_goal_incident(Cpu& c, std::uint32_t actor) {
    for (const auto field : {0x3e, 0x40, 0x42, 0x44, 0x46}) {
        c.set(3, std::uint32_t(c.i16(actor + field)));
        c.set(0, c.r(3) - 1);
        c.put16(actor + field, c.r(0));
    }
    c.set(3, c.u8(actor + 0xc));
    c.set(0, c.r(3) - 1);
    c.put8(actor + 0xc, c.r(0));
}

void resolve_goal_incident(Cpu& c, std::uint32_t actor, std::uint32_t stack, std::uint32_t back) {
    c.set({{3, actor}, {4, 0}});
    call_shot_owner(c, 0x102884d0, back);
    const auto rollback = c.i8(stack + 0x125);
    c.set(0, std::uint32_t(rollback));
    c.compare(0, rollback, 0);
    if (!rollback) return;
    rollback_goal_incident(c, actor);
}
}

static void evaluate_goal_incident_at(Cpu& c, std::uint32_t query, std::uint32_t skip, bool draw) {
    const auto actor = c.r(15), stack = c.sp();
    if (draw) {
        const auto sample = draw_shot_modulo_three(c, query - 0x38);
        c.set(0, sample);
        c.compare(0, std::int32_t(sample), 0);
        if (!sample) {
            c.s.pc = skip;
            return;
        }
    }
    c.set(3, c.u32(actor + 0x1a4));
    c.call(0x101d6980, query + 0x8);
    const auto current = std::int8_t(c.r(3));
    c.set(0, std::uint32_t(current));
    c.compare(0, current, -1);
    if (current <= -1) {
        c.s.pc = skip;
        return;
    }
    report_incident(c, actor, 0x20ed, query + 0x3c);
    c.set(3, c.u32(actor + 0x1a4));
    c.call(0x101d6980, query + 0x44);
    c.set(0, c.r(3));
    c.set(3, c.u32(actor + 0x1a4));
    c.set(13, c.r(0));
    c.call(0x101d5980, query + 0x54);
    c.set(0, c.u8(actor + 0x27));
    c.set(3, std::uint32_t(std::int8_t(c.r(13))));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (c.cr(0) & 2) {
        report_incident(c, actor, 0x20df, query + 0x90);
        if (condition_from_goal_difference(c, stack, query + 0xd8)) report_incident(c, actor, 0x20d9, query + 0x244);
        update_incident(c, actor, 0xe4, query + 0x258);
    } else {
        if (equal_or_one_ahead(c, stack) && draws_opposing_incident(c, query + 0x2b0))
            report_incident(c, actor, 0x20d9, query + 0x314);
        update_incident(c, actor, 0xa0, query + 0x328);
    }
    resolve_goal_incident(c, actor, stack, query + 0x334);
    c.s.pc = incident_exit;
}

void evaluate_goal_incident(Cpu& c) {
    // These original blocks share the incident body and differ at the no-event exit.
    switch (c.s.pc) {
    case 0x10264c1c: evaluate_goal_incident_at(c, 0x10264c74, 0x10265000, true); return;
    case 0x102651bc: evaluate_goal_incident_at(c, 0x10265214, 0x102655a0, true); return;
    case 0x10265618: evaluate_goal_incident_at(c, 0x10265670, 0x102659fc, true); return;
    case 0x10265f10: evaluate_goal_incident_at(c, 0x10265f68, 0x102662f4, true); return;
    case 0x10266350: evaluate_goal_incident_at(c, 0x102663a8, 0x10266734, true); return;
    case 0x10266d68: evaluate_goal_incident_at(c, 0x10266d68, 0x102670f4, false); return;
    default: throw std::logic_error("Unknown goal incident continuation " + hex_address(c.s.pc));
    }
}

namespace {
bool retry_reason_is(Cpu& c, unsigned reason) {
    const auto value = std::int8_t(c.r(17));
    c.set(0, std::uint32_t(value));
    c.compare(0, value, std::int32_t(reason));
    return (c.cr(0) & 2) != 0;
}

bool replay_shot_setup(Cpu& c, std::uint32_t stack, unsigned offset) {
    const auto flag = c.i8(stack + offset);
    c.set(0, std::uint32_t(flag));
    c.compare(0, flag, 0);
    return flag != 0;
}

void restore_repeated_shot(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    c.set(5, c.u32(actor + 0x1a4));
    c.set(4, 0);
    c.set(3, c.toc(-0x51e0));
    c.set(0, 0);
    c.put32(c.r(5) + 0x310c, 0);
    c.set(4, actor);
    c.set_f(1, c.f32(c.r(3) + 0xa0));
    c.set(3, c.u32(actor + 0x1a4));
    c.put32(c.r(3) + 0x3108, 0);
    for (const auto& [slot, field] : {std::pair{0x138u, 0x310cu}, std::pair{0x134u, 0x3108u}}) {
        c.set(0, c.u32(stack + slot));
        c.set(5, c.u32(actor + 0x1a4));
        c.set(3, c.u32(c.r(5) + field));
        c.set(0, c.r(3) + c.r(0));
        c.put32(c.r(5) + field, c.r(0));
    }
    c.set(3, c.u32(actor + 0x1a4));
    c.call(0x101d6780, 0x102650fc);
    report_shot_event(c, actor, 0x20f7, 0x10265124);
    const bool rollback = replay_shot_setup(c, stack, 0x125);
    c.set(0, std::uint32_t(c.i16(stack + 0x12e)));
    c.put16(actor + 0x3a, c.r(0));
    if (rollback) rollback_goal_incident(c, actor);
    c.set(4, c.u8(stack + 0x12a));
    c.set(3, actor);
    c.set(5, c.u8(stack + 0x129));
    call_shot_owner(c, 0x102374f0, 0x10265190);
    continue_shot_retry(c, actor, stack, 0x10265198);
}
}

void choose_goal_incident_retry(Cpu& c) {
    if (c.s.pc == 0x102651b8) {
        c.s.pc = incident_exit;
        return;
    }
    const auto actor = c.r(15), stack = c.sp();
    if (retry_reason_is(c, 3) || retry_reason_is(c, 4)) {
        c.s.pc = 0x10266d44;
        return;
    }
    if (!replay_shot_setup(c, stack, 0x128)) {
        c.set(0, c.u8(actor + 0xc));
        c.compare_logical(0, c.r(0), 3);
        if (c.cr(0) & 2) {
            const auto trait = c.i8(actor + 0x31);
            c.set(0, std::uint32_t(trait));
            c.compare(0, trait, 0);
            if (trait) report_shot_event(c, actor, 0x2011, 0x10265064);
            else report_shot_event(c, actor, 0x1fd5, 0x10265090);
        }
    }
    if (retry_reason_is(c, 5) || retry_reason_is(c, 6)) restore_repeated_shot(c, actor, stack);
    else c.s.pc = 0x102651bc;
}
}
