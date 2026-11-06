#include "match_pl_shot_event_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static bool compare_rating_value(Cpu& c, std::int32_t value, std::int32_t target = 0) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, target);
    return (c.cr(0) & 2) != 0;
}

static std::uint32_t reload_alternate_actor(Cpu& c, std::uint32_t actor) {
    c.set(3, c.u32(actor + 0x1a4) + 0x10000);
    c.set(3, c.u32(c.r(3) - 0x1fd4));
    return c.r(3);
}

static bool records_shot_for_actor(Cpu& c, std::uint32_t actor) {
    c.set(3, c.u8(c.sp() + 0x1cf));
    c.set(0, 0);
    c.compare(0, std::int32_t(c.r(3)), 0x33);
    bool record = (c.cr(0) & 2) != 0;
    if (!record) {
        reload_alternate_actor(c, actor);
        c.compare_logical(0, c.r(3), 0);
        record = (c.cr(0) & 2) != 0;
    }
    if (!record) {
        c.set(3, c.u8(reload_alternate_actor(c, actor) + 0x105));
        c.compare(0, std::int32_t(c.r(3)), 4);
        if (c.cr(0) & 2) {
            c.set(3, std::uint32_t(c.i8(reload_alternate_actor(c, actor) + 0x106)));
            c.compare(0, std::int32_t(c.r(3)), 0);
            record = (c.cr(0) & 2) != 0;
            if (!record) {
                c.set(3, c.u8(reload_alternate_actor(c, actor) + 0x106));
                c.compare(0, std::int32_t(c.r(3)), 11);
                record = (c.cr(0) & 2) != 0;
            }
        }
    }
    return !compare_rating_value(c, record);
}

static bool was_on_target(Cpu& c) {
    return compare_rating_value(c, std::int8_t(c.r(13)), 1) ||
           compare_rating_value(c, std::int8_t(c.r(13)), 14) ||
           compare_rating_value(c, std::int8_t(c.r(13)), 15);
}

static void credit_target_rating(Cpu& c, std::uint32_t actor) {
    c.set(3, c.u8(actor + 0xb));
    const bool scaled = std::int8_t(c.r(13)) == 14 || std::int8_t(c.r(13)) == 15;
    compare_rating_value(c, std::int8_t(c.r(13)), 14);
    c.set(0, c.r(3) + 1);
    c.put8(actor + 0xb, c.r(0));
    if (!(c.cr(0) & 2)) compare_rating_value(c, std::int8_t(c.r(13)), 15);

    const auto stack = c.sp();
    c.set(0, c.u8(stack + 0x1cf));
    c.set(3, 220);
    c.set(4, std::uint32_t(c.i16(actor + 0x36)));
    c.compare(0, std::int32_t(c.r(0)), 0x33);
    if (scaled) c.set_f(2, c.f64(c.r(27) + 0x28));
    if (c.cr(0) & 2) c.set(3, 400);
    if (scaled) {
        Fpu fp(c);
        c.set(3, c.r(3) ^ 0x80000000u);
        c.set(0, 0x43300000);
        c.put32(stack + 0x108, c.r(3));
        c.set_f(1, c.f64(c.r(27) + 0x80));
        c.put32(stack + 0x104, c.r(0));
        c.set_f(0, fp.fsub(c.f64(stack + 0x104), c.f(1)));
        c.set_f(0, fp.fmul(c.f(2), c.f(0)));
        c.put64(stack + 0x10c, fp.fctiwz(0, c.f(0)));
        c.set(0, c.u32(stack + 0x110));
    } else c.set(0, c.r(3));
    c.set(0, c.r(4) + c.r(0));
    c.put16(actor + 0x36, c.r(0));
}

static void credit_shot_rating(Cpu& c, std::uint32_t actor) {
    c.set(4, c.u8(actor + 0xa));
    c.set(3, -150);
    c.set(0, c.r(4) + 1);
    c.put8(actor + 0xa, c.r(0));
    c.set(0, c.u8(c.sp() + 0x1cf));
    c.set(4, std::uint32_t(c.i16(actor + 0x36)));
    c.compare(0, std::int32_t(c.r(0)), 0x33);
    if (c.cr(0) & 2) c.set(3, -1000);
    c.set(0, std::uint32_t(std::int8_t(c.r(13))));
    c.set(3, c.r(4) + c.r(3));
    c.compare(0, std::int32_t(c.r(0)), 1);
    c.put16(actor + 0x36, c.r(3));
    if ((c.cr(0) & 2) || compare_rating_value(c, std::int8_t(c.r(13)), 14) ||
        compare_rating_value(c, std::int8_t(c.r(13)), 15)) credit_target_rating(c, actor);
}

static std::int16_t missed_shot_cost(Cpu& c, std::uint32_t actor) {
    const auto adjusted = c.r(16) - std::uint32_t(c.i16(actor + 0x3a));
    c.s.wr<1>(reg::xer_ca, adjusted <= 240);
    return std::int16_t(240 - adjusted);
}

static void rate_missed_shot(Cpu& c, std::uint32_t actor) {
    const auto estimate = missed_shot_cost(c, actor);
    compare_rating_value(c, estimate);
    c.set(0, (c.cr(0) & 8) ? 0 : std::uint32_t(missed_shot_cost(c, actor)));
    c.set(23, std::uint32_t(std::int16_t(c.r(0))));
    c.compare(0, std::int32_t(c.r(23)), 200);
    if (c.cr(0) & 4) c.set(23, 200);
    c.set(0, std::uint32_t(c.i16(actor + 0x36)) - c.r(23));
    c.put16(actor + 0x36, c.r(0));
    c.set(3, c.u32(actor + 0x6c));
    const auto bound = c.i8(c.r(3) + 0x66);
    c.set(21, std::uint32_t(bound));
    draw_narrow_shot_sample(c, bound, 14, 0x1025b720);
    c.compare(0, std::int32_t(c.r(14)), 0);
    if (!(c.cr(0) & 2)) return;

    c.set(0, 0u - std::uint32_t(std::int16_t(c.r(23))));
    c.set(3, actor);
    const auto high = std::int32_t((std::int64_t(std::int32_t(c.r(0))) * 0x51eb851f) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & 15));
    const auto shifted = std::uint32_t(high >> 4);
    c.set(0, shifted + (shifted >> 31));
    c.set(4, std::uint32_t(std::int8_t(c.r(0))));
    call_shot_owner(c, 0x10288f30, 0x1025b890);
    if (!compare_rating_value(c, std::int8_t(c.r(3)))) {
        c.set({{3, actor}, {4, 0}});
        call_shot_owner(c, 0x1022ba10, 0x1025b8a4);
    }
}

void account_shot_flight(Cpu& c) {
    const auto actor = c.r(15);
    if (compare_rating_value(c, std::int8_t(c.r(17)), 3) ||
        compare_rating_value(c, std::int8_t(c.r(17)), 4)) {
        c.set(0, c.u8(actor + 0x27));
        c.set(3, c.u32(actor + 0x1a4));
        c.set(0, std::uint32_t(std::int8_t(c.r(0))));
        c.set(4, c.r(3) + c.r(0));
        c.set(3, c.u8(c.r(4) + 0x1c2));
        c.set(0, c.r(3) + 1);
        c.put8(c.r(4) + 0x1c2, c.r(0));
    } else if (compare_rating_value(c, std::int8_t(c.r(17)), 1)) {
        if (compare_rating_value(c, c.u8(c.sp() + 0x1cf), 0x33)) {
            c.set(3, c.u8(actor + 0xd));
            c.set(0, c.r(3) + 1);
            c.put8(actor + 0xd, c.r(0));
        }
        if (records_shot_for_actor(c, actor)) credit_shot_rating(c, actor);
        if (!was_on_target(c)) rate_missed_shot(c, actor);
    }
    c.s.pc = 0x1025b8a4;
}
}
