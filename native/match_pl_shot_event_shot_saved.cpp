#include "match_pl_shot_event_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

namespace {
constexpr auto epilogue = 0x1026a190;

bool equals(Cpu& c, std::int32_t value, std::int32_t bound = 0) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, bound);
    return value == bound;
}

bool reason_is(Cpu& c, int reason) {
    return equals(c, std::int8_t(c.r(17)), reason);
}

void report_keeper_save_event(Cpu& c, std::uint32_t actor, std::uint32_t keeper, unsigned event, std::uint32_t back) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, event);
    c.set(7, c.u8(keeper + 0x27));
    c.set(8, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(9, UINT32_MAX);
    c.set(6, c.u8(keeper + 0x2f));
    c.set(10, 0);
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, back);
}

void report_keeper_optional_event(Cpu& c, std::uint32_t actor, std::uint32_t keeper, std::uint32_t back) {
    for (const auto event : {0x219fu, 0x21a0u, 0x21b0u, 0x21b3u, 0x21b4u, 0x21b5u}) {
        c.set({{3, keeper}, {4, event}});
        call_shot_owner(c, 0x10289d50, back);
        if (!equals(c, std::int8_t(c.r(3)))) {
            report_keeper_save_event(c, actor, keeper, event, back + 0x30);
            return;
        }
        back += 0x40;
    }
}

void report_actor_optional_event(Cpu& c, std::uint32_t actor, unsigned event, std::uint32_t back) {
    c.set({{3, actor}, {4, event}});
    call_shot_owner(c, 0x10289d50, back);
    if (!equals(c, std::int8_t(c.r(3)))) report_shot_event(c, actor, event, back + 0x30);
}

void describe_save_reason(Cpu& c, std::uint32_t actor, std::uint32_t stack, std::uint32_t common_return) {
    const bool four = common_return == 0x102688ac ? (c.cr(0) & 2) != 0 : reason_is(c, 4);
    if (four || reason_is(c, 3)) {
        report_shot_event(c, actor, 0x2008, common_return);
        if (reason_is(c, 3)) {
            c.set(0, 1);
            c.put8(stack + 0x126, 1);
        } else report_shot_event(c, actor, 0x2174, common_return + 0x40);
    } else if (equals(c, c.u8(stack + 0x1cf), 0x33)) {
        if (reason_is(c, 1)) report_shot_event(c, actor, 0x1fce, common_return + 0x84);
        c.set(0, 1);
        c.put8(stack + 0x126, 1);
    }
}

void describe_keeper_save(Cpu& c, std::uint32_t actor, std::uint32_t keeper, std::uint32_t stack,
                          std::uint32_t common_return, bool strong) {
    describe_save_reason(c, actor, stack, common_return);
    if (!equals(c, c.i8(stack + 0x126))) report_keeper_save_event(c, actor, keeper, 0x2122, common_return + 0xc0);
    else if (!equals(c, c.i8(stack + 0x123))) {
        report_keeper_save_event(c, actor, keeper, 0x2111, common_return + 0xf8);
        report_keeper_optional_event(c, actor, keeper, common_return + 0x104);
    } else if (!equals(c, c.i8(stack + 0x122))) {
        report_keeper_save_event(c, actor, keeper, 0x210f, common_return + 0x2ac);
    } else if (strong) {
        report_keeper_save_event(c, actor, keeper, 0x1fe6, 0x10267e94);
    } else if (equals(c, c.u8(stack + 0x1cf), 0x1a)) {
        report_keeper_save_event(c, actor, keeper, 0x21d0, 0x102684ac);
    } else report_keeper_save_event(c, actor, keeper, 0x1fe4, 0x102684d8);
}

void adjust_keeper_save_rating(Cpu& c, std::uint32_t keeper, std::uint32_t stack, unsigned standard_gain) {
    if (reason_is(c, 5) || reason_is(c, 6) || reason_is(c, 2)) return;
    c.set(0, c.u8(stack + 0x1cf));
    c.set(3, standard_gain);
    c.set(4, std::uint32_t(c.i16(keeper + 0x36)));
    c.compare(0, std::int32_t(c.r(0)), 0x33);
    if (c.cr(0) & 2) c.set(3, 1000);
    c.set(0, c.r(4) + c.r(3));
    c.put16(keeper + 0x36, c.r(0));
}

bool pressure_save_comment(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    return has_shot_pressure(c, actor, stack) ||
           (!equals(c, c.i8(stack + 0x124)) && equals(c, c.i8(stack + 0x128)));
}

void report_saved_distance(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    if (!equals(c, c.i8(stack + 0x128))) return;
    equals(c, c.i8(stack + 0x127), 25);
    if ((c.cr(0) & 8) || !reason_is(c, 1)) return;
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, 0x1f9e);
    c.set(0, c.u8(stack + 0x127));
    c.set(8, UINT32_MAX);
    c.set(7, c.u8(actor + 0x27));
    c.set(9, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(10, std::uint32_t(std::int8_t(c.r(0))));
    c.set(6, c.u8(actor + 0x2f));
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, 0x10268174);
}

void update_keeper_saved_ball(Cpu& c, std::uint32_t actor, std::uint32_t keeper, std::uint32_t stack) {
    advance_shot_stream(c, 0x10267fd8);
    c.set(3, c.u32(c.r(31)));
    c.set(4, 0x55550000);
    c.set(0, c.u32(c.r(28)));
    c.set(4, 0x55555556);
    c.set(5, c.u16(c.r(3)));
    c.set(3, keeper);
    const auto sample = c.r(5) + c.r(0);
    c.set(5, sample);
    const auto high = std::uint32_t((std::int64_t(std::int32_t(sample)) * 0x55555556) >> 32);
    c.set(0, sample - (high + (high >> 31)) * 3 + 1);
    c.set(4, std::uint32_t(std::int8_t(c.r(0))));
    call_shot_owner(c, 0x10285ed0, 0x1026801c);

    c.set(4, c.u8(stack + 0x1cf));
    c.set(0, 0x33);
    c.set(3, 0x33);
    c.set(0, c.u8(actor + 0x27));
    c.set(4, std::uint32_t(std::int8_t(c.r(4))));
    c.set(5, c.u32(actor + 0x1a4));
    c.set(6, c.r(3) - c.r(4));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(3, c.r(4) - c.r(3));
    c.set(7, c.u32(actor + 0x1a4));
    c.set(3, c.r(6) | c.r(3));
    c.set(0, c.r(0) == 0 ? 32 : 0);
    c.set(6, c.r(3) >> 31);
    c.set(5, c.u8(c.r(5) + 0x791e));
    c.set(9, c.r(6));
    c.set(4, c.u8(c.r(7) + 0x791d));
    c.set(0, c.r(0) >> 5);
    c.set(3, c.u32(actor + 0x1a4));
    c.set({{7, c.r(0)}, {6, 0}, {8, 0}, {10, 0}});
    c.call(0x101f1100, 0x10268078);
    c.set(0, c.u8(stack + 0x126));
    c.set(4, 1);
    c.set(3, c.u32(actor + 0x1a4));
    const auto set_piece = std::int8_t(c.r(0));
    equals(c, set_piece);
    c.put8(c.r(3) + 0x1e6, 1);
    if (set_piece) return;
    c.set(3, c.u32(actor + 0x1a4));
    if (equals(c, c.i8(c.r(3) + 0x7928)) || reason_is(c, 4)) return;
    if (pressure_save_comment(c, actor, stack)) report_shot_event(c, actor, 0x2020, 0x1026811c);
    else report_saved_distance(c, actor, stack);
}
}

void resolve_saved_shot(Cpu& c) {
    const auto actor = c.r(15), keeper = c.r(24), stack = c.sp();
    if (c.s.pc == 0x10267f9c || c.s.pc == 0x102686f0) {
        c.s.pc = epilogue;
        return;
    }
    const bool strong = c.s.pc == 0x10267b6c;
    c.set(3, c.toc(-0x51e0));
    c.set_f(0, c.f32(c.r(3) + (strong ? 0x134 : 0x138)));
    Fpu(c).fcmp(0, c.f(31), c.f(0));
    if (!(c.cr(0) & 4)) {
        c.s.pc = strong ? 0x10268178 : 0x102687e8;
        return;
    }

    describe_keeper_save(c, actor, keeper, stack, strong ? 0x10267bbc : 0x102681c8, strong);
    if (strong) {
        adjust_keeper_save_rating(c, keeper, stack, 140);
        if (reason_is(c, 5) || reason_is(c, 6)) {
            restore_and_retry_shot(c, actor, stack, 0x10267f5c);
            return;
        }
        if (reason_is(c, 1) || reason_is(c, 2)) update_keeper_saved_ball(c, actor, keeper, stack);
    } else {
        report_actor_optional_event(c, actor, 0x21a1, 0x102684e4);
        report_actor_optional_event(c, actor, 0x21a2, 0x10268520);
        if (equals(c, c.i8(stack + 0x126))) {
            if (pressure_save_comment(c, actor, stack)) report_shot_event(c, actor, 0x2020, 0x102685cc);
            adjust_keeper_save_rating(c, keeper, stack, 120);
        }
        if (!reason_is(c, 4) && !reason_is(c, 3)) {
            if (reason_is(c, 5) || reason_is(c, 6)) {
                restore_and_retry_shot(c, actor, stack, 0x102686b0);
                return;
            }
            place_alternate_shot_outside_pitch(c, actor, stack, 0x10268748, 0x3d39);
            close_alternate_shot(c, actor, 0x102687e4);
        }
    }
    c.s.pc = epilogue;
}

namespace {
void open_final_save(Cpu& c, std::uint32_t keeper, std::uint32_t stack) {
    advance_shot_stream(c, 0x10268808);
    c.set(3, c.u32(c.r(31)));
    c.set(4, 0x55550000);
    c.set(0, c.u32(c.r(28)));
    c.set(4, 0x55555556);
    c.set(5, c.u16(c.r(3)));
    c.set(3, keeper);
    const auto sample = c.r(5) + c.r(0);
    c.set(5, sample);
    const auto high = std::uint32_t((std::int64_t(std::int32_t(sample)) * 0x55555556) >> 32);
    c.set(0, sample - (high + (high >> 31)) * 3);
    c.set(4, std::uint32_t(std::int8_t(c.r(0))));
    call_shot_owner(c, 0x10285ed0, 0x10268848);

    c.set(0, c.u8(stack + 0x1cf));
    c.set(3, 200);
    c.set(4, std::uint32_t(c.i16(keeper + 0x36)));
    c.compare(0, std::int32_t(c.r(0)), 0x33);
    if (c.cr(0) & 2) c.set(3, 1300);
    c.set(0, std::uint32_t(std::int8_t(c.r(17))));
    c.set(3, c.r(4) + c.r(3));
    c.compare(0, std::int32_t(c.r(0)), 4);
    c.put16(keeper + 0x36, c.r(3));
}

void describe_final_keeper_save(Cpu& c, std::uint32_t actor, std::uint32_t keeper, std::uint32_t stack) {
    describe_save_reason(c, actor, stack, 0x102688ac);
    if (!equals(c, c.i8(stack + 0x126))) {
        report_keeper_save_event(c, actor, keeper, 0x2122, 0x1026896c);
    } else if (!equals(c, c.i8(stack + 0x123))) {
        report_keeper_save_event(c, actor, keeper, 0x2112, 0x102689a4);
        report_keeper_optional_event(c, actor, keeper, 0x102689b0);
    } else if (!equals(c, c.i8(stack + 0x122))) {
        const bool special = equals(c, c.u8(stack + 0x1cf), 0x1a) && equals(c, c.i8(stack + 0x46));
        report_keeper_save_event(c, actor, keeper, special ? 0x21d0 : 0x2110,
                                 special ? 0x10268b70 : 0x10268b9c);
    } else {
        const bool special = equals(c, c.u8(stack + 0x1cf), 0x1a) && equals(c, c.i8(stack + 0x46));
        report_keeper_save_event(c, actor, keeper, special ? 0x21d0 : 0x1fe5,
                                 special ? 0x10268be0 : 0x10268c0c);
        report_available_miss_message(c, actor, 0x10268c18);
    }
}

bool was_powerful_saved_shot(Cpu& c, std::uint32_t actor, std::uint32_t stack) {
    const auto first = c.i8(stack + 0x122);
    c.set(5, first ? 200 : 0);
    const auto second = c.i8(stack + 0x123);
    c.s.wr<1>(reg::xer_ca, second != 0);
    c.set(0, second ? 400 : 0);
    c.set(4, std::uint32_t(c.i16(actor + 0x3a)));
    c.set(3, c.r(0) + c.r(5));
    c.set(0, c.r(3) + 300);
    c.set(6, first ? UINT32_MAX : 0);
    c.compare(0, std::int32_t(c.r(4)), std::int32_t(c.r(0)));
    return (c.cr(0) & 4) || equals(c, c.u8(stack + 0x1cf), 0x3a);
}

void describe_final_save_context(Cpu& c, std::uint32_t actor, std::uint32_t stack, bool power) {
    equals(c, c.i16(stack + 0x12c), -135);
    if (!(c.cr(0) & 8) || equals(c, c.u8(stack + 0x1cf), 0x3a)) return;
    draw_miss_context(c, power ? 0x10268ed4 : 0x102697f8, 3);
    c.compare(0, std::int32_t(c.r(13)), 0);
    if (c.cr(0) & 2) return;
    if (equals(c, std::int8_t(c.r(18)), 2)) {
        report_double_shot_context(c, power ? 0x102692f4 : 0x10269c18);
        return;
    }
    if (c.cr(0) & 8) {
        c.compare(0, std::int32_t(c.r(0)), 1);
        if (!(c.cr(0) & 8)) {
            report_single_shot_context(c, power ? 0x102690b8 : 0x102699dc);
            return;
        }
    }
    report_shot_event(c, actor, 0x2132, power ? 0x102697a0 : 0x1026a0c4);
}
}

void resolve_final_saved_shot(Cpu& c) {
    if (c.s.pc == 0x1026a184) {
        c.s.pc = epilogue;
        return;
    }
    const auto actor = c.r(15), keeper = c.r(24), stack = c.sp();
    open_final_save(c, keeper, stack);
    describe_final_keeper_save(c, actor, keeper, stack);
    report_actor_optional_event(c, actor, 0x21a1, 0x10268d14);
    report_actor_optional_event(c, actor, 0x21a2, 0x10268d50);
    if (equals(c, c.i8(stack + 0x126))) {
        if (pressure_save_comment(c, actor, stack)) report_shot_event(c, actor, 0x2020, 0x10268dfc);
        else {
            const bool power = was_powerful_saved_shot(c, actor, stack);
            if (power) report_power_miss(c, actor, 0x1fdd, 0x10268e80);
            describe_final_save_context(c, actor, stack, power);
        }
    }
    if (reason_is(c, 5) || reason_is(c, 6)) {
        restore_and_retry_shot(c, actor, stack, 0x1026a144);
        return;
    }
    c.set(3, keeper);
    call_shot_owner(c, 0x1022e3a0, epilogue);
    c.s.pc = epilogue;
}
}
