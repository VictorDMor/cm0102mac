#include "match_pl_shot_event_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static bool narration_equals(Cpu& c, std::int32_t value, std::int32_t bound = 0) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, bound);
    return (c.cr(0) & 2) != 0;
}

bool has_shot_pressure(Cpu& c, std::uint32_t actor, std::uint32_t stack, unsigned threshold) {
    Fpu fp(c);
    c.set(3, c.u32(actor + 0x1a4));
    c.set(0, 0x43300000);
    c.put32(stack + 0xfc, c.r(0));
    c.set(0, c.u32(c.r(3) + 0x3108));
    c.set_f(2, c.f64(c.r(27) + 0x80));
    c.set(0, c.r(0) ^ 0x80000000u);
    c.set_f(0, c.f64(c.r(27) + threshold));
    c.put32(stack + 0x100, c.r(0));
    c.set_f(1, fp.fsub(c.f64(stack + 0xfc), c.f(2)));
    fp.fcmp(0, c.f(1), c.f(0));
    return (c.cr(0) & 4) != 0;
}

static std::uint32_t miss_context_square(Cpu& c) {
    c.set(3, std::uint32_t(std::int8_t(c.r(18))));
    c.set(0, std::uint32_t(std::int8_t(c.r(18))));
    return c.r(3) * c.r(0);
}

void draw_miss_context(Cpu& c, std::uint32_t refill_back, unsigned base) {
    c.set(3, miss_context_square(c));
    c.s.wr<1>(reg::xer_ca, 0);
    narration_equals(c, std::int32_t(c.r(3) + base));
    advance_shot_stream(c, refill_back);
    // A squared signed byte plus three or four is in [3, 16388]. The zero
    // and wide outcomes cannot be selected in the original.
    for (const auto limit : {65535, -65535}) {
        c.set(4, miss_context_square(c));
        c.set(3, std::uint32_t(limit));
        narration_equals(c, std::int32_t(c.r(4) + base), limit);
    }
    c.set(4, miss_context_square(c));
    c.set(6, c.u32(c.r(31)));
    c.set(5, c.u32(c.r(28)));
    c.set(3, std::uint32_t(std::int8_t(c.r(18))));
    c.set(6, c.u16(c.r(6)));
    c.set(0, std::uint32_t(std::int8_t(c.r(18))));
    c.set(6, c.r(6) + c.r(5));
    c.set(5, c.r(4) + base);
    const auto quotient = std::uint32_t(divide_signed<4>(c.r(6), c.r(5)));
    c.set(3, c.r(3) * c.r(0));
    c.set(4, quotient * c.r(5));
    c.s.wr<1>(reg::xer_ca, 0);
    narration_equals(c, std::int32_t(c.r(3) + base));
    c.set(13, c.r(6) - c.r(4));
    c.set(0, c.r(13));
}

void choose_miss_narration(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    if (!has_shot_pressure(c, actor, stack)) {
        c.s.pc = 0x1025e6d0;
        return;
    }
    narration_equals(c, c.i16(stack + 0x12c), -135);
    if (!(c.cr(0) & 8) || narration_equals(c, c.u8(stack + 0x1cf), 0x3a)) {
        c.s.pc = 0x1025e65c;
        return;
    }
    draw_miss_context(c, 0x1025d7fc);
    c.compare(0, std::int32_t(c.r(13)), 0);
    if (c.cr(0) & 2) {
        c.s.pc = 0x1025e6a4;
        return;
    }
    if (narration_equals(c, std::int8_t(c.r(18)), 2)) c.s.pc = 0x1025dec0;
    else if (c.cr(0) & 4) c.s.pc = 0x1025e630;
    else {
        c.compare(0, std::int32_t(c.r(0)), 1);
        c.s.pc = (c.cr(0) & 8) ? 0x1025e630 : 0x1025d9d0;
    }
}

void choose_low_power_miss(Cpu& c) {
    const auto stack = c.sp();
    narration_equals(c, c.i16(stack + 0x12c), -135);
    if (!(c.cr(0) & 8) || narration_equals(c, c.u8(stack + 0x1cf), 0x3a)) {
        c.s.pc = 0x1025f858;
        return;
    }
    draw_miss_context(c, 0x1025ecc8);
    c.compare(0, std::int32_t(c.r(13)), 0);
    if (c.cr(0) & 2) {
        c.s.pc = 0x1025f858;
        return;
    }
    if (narration_equals(c, std::int8_t(c.r(18)), 2)) c.s.pc = 0x1025f38c;
    else if (c.cr(0) & 4) c.s.pc = 0x1025f830;
    else {
        c.compare(0, std::int32_t(c.r(0)), 1);
        c.s.pc = (c.cr(0) & 8) ? 0x1025f830 : 0x1025ee9c;
    }
}

void choose_target_shot_narration(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    constexpr std::uint32_t finish = 0x10262154;
    if (narration_equals(c, c.u8(stack + 0x1cf), 0x33)) {
        c.s.pc = finish;
        return;
    }
    if (narration_equals(c, c.i8(stack + 0x124)) || !narration_equals(c, c.i8(stack + 0x128))) {
        c.s.pc = 0x10260284;
        return;
    }
    report_shot_event(c, actor, 0x201e, 0x1025f960);
    narration_equals(c, c.i16(stack + 0x12c), -135);
    if (!(c.cr(0) & 8) || narration_equals(c, c.u8(stack + 0x1cf), 0x3a)) {
        c.s.pc = finish;
        return;
    }
    draw_miss_context(c, 0x1025f9b4);
    c.compare(0, std::int32_t(c.r(13)), 0);
    if (c.cr(0) & 2) {
        c.s.pc = finish;
        return;
    }
    if (narration_equals(c, std::int8_t(c.r(18)), 2)) c.s.pc = 0x1025fdb4;
    else if (c.cr(0) & 4) c.s.pc = 0x10260258;
    else {
        c.compare(0, std::int32_t(c.r(0)), 1);
        c.s.pc = (c.cr(0) & 8) ? 0x10260258 : 0x1025fb88;
    }
}

void describe_simple_miss(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    if (c.s.pc == 0x1025e630) report_shot_event(c, actor, 0x2132, 0x1025e658);
    else if (c.s.pc == 0x1025e65c && narration_equals(c, c.u8(stack + 0x1cf), 0x3a)) {
        narration_equals(c, c.i16(stack + 0x12c), -135);
        if (!(c.cr(0) & 8) && narration_equals(c, std::int8_t(c.r(18))))
            report_shot_event(c, actor, 0x219a, 0x1025e6a4);
    }
    report_shot_event(c, actor, 0x201f, 0x1025e6cc);
    c.s.pc = 0x1025f858;
}
}
