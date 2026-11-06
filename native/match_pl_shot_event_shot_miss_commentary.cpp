#include "match_pl_shot_event_detail.hpp"

#include <array>

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

static bool miss_value_equals(Cpu& c, std::int32_t value, std::int32_t target) {
    c.set(0, std::uint32_t(value));
    c.compare(0, value, target);
    return (c.cr(0) & 2) != 0;
}

bool report_available_miss_message(Cpu& c, std::uint32_t actor, std::uint32_t first_query_return) {
    constexpr std::array<unsigned, 4> codes{0x219e, 0x21b1, 0x21b2, 0x21b7};
    auto back = first_query_return;
    for (const auto code : codes) {
        c.set({{3, actor}, {4, code}});
        call_shot_owner(c, 0x10289d50, back);
        if (!miss_value_equals(c, std::int8_t(c.r(3)), 0)) {
            report_shot_event(c, actor, code, back + 0x30);
            return true;
        }
        back += 0x40;
    }
    return false;
}

void describe_missed_shot(Cpu& c) {
    const auto actor = c.r(15), stack = c.sp();
    if (miss_value_equals(c, std::int8_t(c.r(17)), 4) || miss_value_equals(c, std::int8_t(c.r(17)), 3)) {
        report_shot_event(c, actor, 0x2008, 0x1025c2cc);
        if (miss_value_equals(c, std::int8_t(c.r(17)), 4)) report_shot_event(c, actor, 0x2174, 0x1025c300);
    } else if (miss_value_equals(c, c.u8(stack + 0x1cf), 0x33)) {
        report_shot_event(c, actor, 0x1fce, 0x1025c338);
    }
    if (!miss_value_equals(c, c.u8(stack + 0x1cf), 0x33)) {
        c.s.pc = 0x1025c60c;
        return;
    }
    const auto category = std::uint32_t(std::int8_t(c.r(13)));
    c.set(0, category);
    c.compare_logical(0, category, 13);
    c.s.pc = 0x1025d164;
    if (c.cr(0) & 4) return;
    c.set(3, c.toc(-0x523c));
    c.set(0, category * 4);
    c.s.pc = 0x1025c358;
    const auto target = c.u32(c.r(3) + c.r(0));
    c.set(3, target);
    c.s.wr<4>(reg::ctr, target);
    switch (target) {
    case 0x1025c364:
        report_shot_event(c, actor, 0x2125, 0x1025c38c, true);
        report_available_miss_message(c, actor, 0x1025c398);
        break;
    case 0x1025c48c:
        report_shot_event(c, actor, 0x2126, 0x1025c4b4, true);
        report_available_miss_message(c, actor, 0x1025c4c0);
        break;
    case 0x1025c5b4:
        report_shot_event(c, actor, 0x2124, 0x1025c5dc, true);
        break;
    case 0x1025c5e0:
        report_shot_event(c, actor, 0x2123, 0x1025c608, true);
        break;
    case 0x1025d164: break;
    default: c.s.pc = target; return;
    }
    c.s.pc = 0x1025d164;
}

static void report_keeper_miss(Cpu& c, std::uint32_t actor, std::uint32_t keeper,
                               unsigned code, std::uint32_t back) {
    c.set(3, c.u32(actor + 0x1a4));
    c.set(4, code);
    c.set(7, c.u8(keeper + 0x27));
    c.set(8, UINT32_MAX);
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set(9, UINT32_MAX);
    c.set(6, c.u8(keeper + 0x2f));
    c.set(10, 0);
    c.set(5, c.u32(actor + 0x1a4));
    c.call(0x10201180, back);
}

static void maybe_report_keeper_miss(Cpu& c, std::uint32_t actor, std::uint32_t phase) {
    const auto stack = c.sp(), keeper = c.r(24);
    if (miss_value_equals(c, c.u8(stack + 0x1cf), 0x33) ||
        miss_value_equals(c, c.u8(stack + 0x1cf), 0x3a) ||
        !miss_value_equals(c, c.i8(stack + 0x45), 0)) return;
    draw_shot_modulo_fifty(c, phase + 0x44);
    if (!(c.cr(0) & 2) || !miss_value_equals(c, c.i8(stack + 0x46), 0)) return;
    c.set(3, keeper);
    call_shot_owner(c, 0x10289560, phase + 0x94);
    if (!miss_value_equals(c, std::int16_t(c.r(3)), 1)) return;
    const bool special = miss_value_equals(c, c.u8(stack + 0x1cf), 0x1a);
    report_keeper_miss(c, actor, keeper, special ? 0x21d2 : 0x20d6,
                       phase + (special ? 0xd4 : 0x100));
}

void describe_open_play_miss(Cpu& c) {
    const auto actor = c.r(15);
    const auto category = std::uint32_t(std::int8_t(c.r(13)));
    c.set(0, category);
    c.compare_logical(0, category, 13);
    c.s.pc = 0x1025d164;
    if (c.cr(0) & 4) return;
    c.set(3, c.toc(-0x5240));
    c.set(0, category * 4);
    c.s.pc = 0x1025c620;
    const auto target = c.u32(c.r(3) + c.r(0));
    c.set(3, target);
    c.s.wr<4>(reg::ctr, target);
    constexpr std::uint32_t first = 0x1025ca60, stride = 0x12c, last = 0x1025d03c;
    switch (target) {
    case 0x1025c62c:
        report_shot_event(c, actor, 0x1fbf, 0x1025c654);
        report_available_miss_message(c, actor, 0x1025c660);
        break;
    case 0x1025c754: {
        const bool corner = miss_value_equals(c, std::int8_t(c.r(20)), 5);
        report_shot_event(c, actor, corner ? 0x1fc1 : 0x1fc0, corner ? 0x1025c788 : 0x1025c7b4, true);
        report_available_miss_message(c, actor, 0x1025c7c0);
        break;
    }
    case 0x1025c8b4:
        report_shot_event(c, actor, 0x1fc2, 0x1025c8dc);
        report_available_miss_message(c, actor, 0x1025c8e8);
        break;
    case 0x1025c9dc: report_shot_event(c, actor, 0x1fc3, 0x1025ca04); break;
    case 0x1025ca08: report_shot_event(c, actor, 0x1fc4, 0x1025ca30); break;
    case 0x1025ca34: report_shot_event(c, actor, 0x1fc5, 0x1025ca5c); break;
    case 0x1025d164: break;
    default:
        if (target < first || target > last || (target - first) % stride != 0) {
            c.s.pc = target;
            return;
        }
        maybe_report_keeper_miss(c, actor, target);
        report_shot_event(c, actor, 0x1fc6 + (target - first) / stride, target + 0x128);
        break;
    }
    c.s.pc = 0x1025d164;
}
}
