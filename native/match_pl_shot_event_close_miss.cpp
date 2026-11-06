#include "match_pl_shot_event_detail.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static bool compare_close_miss(Cpu& c, std::int32_t value, std::int32_t against = 0) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, against);
    return (c.cr(0) & 2) != 0;
}

static bool teammate_accepts_close_miss(Cpu& c, std::uint32_t actor) {
    c.set(3, c.u32(actor + 0x70));
    compare_close_miss(c, c.i8(c.r(3) + 0x3f), 10);
    if (!(c.cr(0) & 8)) return false;

    advance_shot_stream(c, 0x1025d540);
    c.set(5, c.u32(c.r(31)));
    c.set(4, 0x2aab0000);
    c.set(3, c.u32(actor + 0x6c));
    c.set(4, 0x2aaaaaab);
    c.set(6, c.u16(c.r(5)));
    c.set(5, c.u32(c.r(28)));
    c.set(0, c.u8(c.r(3) + 0x67));
    const auto sample = c.r(6) + c.r(5);
    c.set(5, sample);
    const auto high = std::uint32_t((std::int64_t(std::int32_t(sample)) * 0x2aaaaaab) >> 32);
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    c.set(4, sample - (high + (high >> 31)) * 6);
    c.set(0, std::uint32_t(std::int8_t(c.r(4) + 10)));
    c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (!(c.cr(0) & 8)) return false;

    const auto teammate = c.r(14);
    c.set(3, c.u32(teammate + 0x6c));
    compare_close_miss(c, c.i8(c.r(3) + 0x67), 10);
    if (!(c.cr(0) & 8)) return false;
    c.set(3, c.u32(teammate + 0x6c));
    const auto bound = c.i8(c.r(3) + 0x67);
    c.set(13, std::uint32_t(bound));
    draw_narrow_shot_sample(c, bound, 16, 0x1025d5dc);
    c.compare(0, std::int32_t(c.r(16)), 0);
    return (c.cr(0) & 2) != 0;
}

static void report_close_miss_teammate(Cpu& c, std::uint32_t actor, std::uint32_t teammate) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, 0x21dd);
    c.set(5, c.u32(actor + 0x1a4));
    c.set(10, 0);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(6, c.u8(actor + 0x2f));
    c.set(7, c.u8(actor + 0x27));
    c.set(8, c.u8(teammate + 0x2f));
    c.set(9, c.u8(teammate + 0x27));
    c.call(0x10201180, 0x1025d748);
}

void choose_close_miss(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    if (compare_close_miss(c, c.u8(stack + 0x1cf), 0x33)) {
        c.s.pc = 0x1025f858;
        return;
    }
    if (compare_close_miss(c, c.i8(stack + 0x124)) || !compare_close_miss(c, c.i8(stack + 0x128))) {
        c.s.pc = 0x1025d778;
        return;
    }
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, actor);
    c.call(0x101fed00, 0x1025d504);
    c.set(14, c.r(3));
    c.compare(0, std::int32_t(c.r(14)), 0);
    if (!(c.cr(0) & 2) && teammate_accepts_close_miss(c, actor))
        report_close_miss_teammate(c, actor, c.r(14));
    else
        report_shot_event(c, actor, 0x201e, 0x1025d774);
    c.s.pc = 0x1025f858;
}
}
